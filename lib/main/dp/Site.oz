functor
import
   Protocol
   Serialization(serializer:Serializer deserializer:Deserializer)
export
   This
define
   
   class Site
      attr
	 shortId
	 reachability
      meth init()
	 reachability := r(version:0 addresses:nil)
      end
      meth getId($)
	 @shortId
      end
      meth setReachability(R)
	 if R.version > @reachability.version then
	    reachability := R
	 end
      end
      meth getReachability($)
	 @reachability
      end
   end
   
   class ThisSite
      attr
	 remotes
	 protocolManager
	 dispatcher
      from Site
      meth init()
	 Site,init()
	 remotes := {Dictionary.new}
	 shortId := {Name.new}
	 protocolManager := {New Protocol.manager init(self)}
	 {Protocol.registerBaseProtocols @protocolManager}
	 dispatcher := {Dictionary.new}
	 {self registerDispatchingTarget(site proc{$ From Msg}
						 case Msg
						 of siteEnquire(to:_ ask:ShortId) then
						    {From sendMessage(siteAnswer(to:site
										 id:ShortId
										 descr:{{self getRemote(ShortId unit $)} getReachability($)}))}
						 [] siteAnswer(to:_ id:ShortId descr:Descr) then
						    {{self getRemote(ShortId unit $)} setReachability(Descr)}
						 end
					      end _)}
      end
      meth getProtocolManager($)
	 @protocolManager
      end
      meth addAddress(A)
	 CurReach = {self getReachability($)} in
	 {self setReachability(r(version:CurReach.version+1 addresses:A|CurReach.addresses))}
      end
      meth getRemote(ShortId KnowingSite $)
	 NewRemote in
	 case {Dictionary.condExchange @remotes ShortId new $ NewRemote}
	 of new then
	    NewRemote = {New RemoteSite init(self ShortId)}
	    if KnowingSite\=unit then {KnowingSite sendMessage(siteEnquire(to:site ask:ShortId))} end
	    NewRemote
	 [] OldRemote then
	    NewRemote = OldRemote
	 end
      end
      meth dispatchMessage(FromSite Msg)
	 if {Record.is Msg}
	    andthen {HasFeature Msg to} then
	    {{Dictionary.condGet @dispatcher Msg.to proc{$ _ _}skip end} FromSite Msg}
	 end
      end
      meth registerDispatchingTarget(Key Proc ?Revoker)
	 NewTarget in
	 case {Dictionary.condExchange @dispatcher Key unit $ NewTarget}
	 of unit then
	    NewTarget = Proc
	    proc {Revoker} {Dictionary.remove @dispatcher Key} end
	 [] OldTarget then
	    NewTarget=OldTarget
	    raise targetAlreadyExist(Key) end
	 end
      end
   end
   This = {New ThisSite init()}
   
   class RemoteSite
      from Site
      attr
	 localSite
	 queuePort
	 queue
	 channel
      meth init(LocalSite ShortId)
	 Site,init()
	 localSite := LocalSite
	 shortId := ShortId
	 channel := unit
	 {Port.new @queue @queuePort}
      end
      meth setChannel(Channel)
	 channel := Channel
	 thread
	    try
	       for while:true do
		  {self receiveBytes({Channel receive($)})}
	       end
	    catch _ then
	       skip
	    end
	 end
	 thread
	    proc {P Ms}
	       case Ms of M|Mt then
		  {Channel send(M)}
		  queue:=Mt
		  {P Mt}
	       end
	    end in
	    try
	       {P @queue}
	    catch _ then
	       skip
	    end
	 end
      end
      meth hasChannel($)
	 @channel\=unit
      end
      meth sendBytes(VBS)
	 {Send @queuePort VBS}
      end
      meth sendMessage(Msg)
	 Self = self in
	 thread
	    {Self sendBytes({{New Serializer init(@localSite Self Msg)} close($)})}
	 end
      end
      meth receiveBytes(VBS)
	 thread
	    {self receiveMessage({{New Deserializer init(@localSite self VBS)} close($)})}
	 end
      end
      meth receiveMessage(Msg)
	 {@localSite dispatchMessage(self Msg)}
      end
   end





   
   fun{GetProtoInt Vbs Idx V ?LIdx}
      B = {VirtualByteString.get Header Idx} in
      if B<128 then
	 LIdx=Idx
	 V+B
      else
	 {GetProtoInt Vbs Idx+1 (V*128)+B-128 LIdx}
      end
   end
   Magic = [&D &s &s 0xDE 0xC0 0xDE &P &B &u &f 0xF0 0x0D &v &1 &. &0]
   MagicLength = 8
   MsgKind = {VirtualString.toCompactString "mozart.pb.Stream"}
   class HighLevelChannel
      attr
	 chan
	 closed: false
      meth init(Channel)
	 chan := Channel
	 thread
	    if {Channel send(Magic error:$)} then {self close} end
	    if {VirtualByteString.toList {self receiveBytes(MagicLength $)}} \= Magic then {self close} end
	    {self send('Hello'())}
	    for until:{self closed($)}
	       H in
	       {self receiveBytes(6 Header)}
	       if {VirtualByteString.get Header 0}\=10 then {self close()} end
	       H = {GetProtoInt Header 1 0 T}
	       if H mod 8 \= 2 then
		  {self close}
	       else
		  {self receivedBytes(Header#{self receiveBytes(H div 8 - 5 + T $)})}
	       end
	    end
	 end
      end
      meth close()
	 {@chan close()}
	 closed := true
      end
      meth closed($)
	 @closed
      end
      meth receiveBytes(Size ?Vbs)
	 if {Channel receive(MagicTest size:MagicLength error:$)} then {self close} end
      end
      meth receivedBytes(Vbs)
	 if @closed then skip else
	    {self received({PBuf.bytesToRecord P MsgKind Vbs}.elements.1)}
	 end
      end
   end

   proc{Acks Dic Miss Last}
      proc{Loop Keys Miss Last}
	 case Keys#Miss
	 of _#nil then
	    for K in Keys while:K=<Last do {Dictionary.remove Dic K} end
	 [] nil#_ then
	    skip
	 [] (Hk|Tk)#(Hm|Tm) then
	    if Hk>Last then
	       skip
	    elseif Hk<Hm then
	       {Dictionary.remove Dic Hk}
	       {Loop Tk Miss Last}
	    elseif Hk==Hm then
	       {Loop Tk Tm Last}
	    else
	       {System.showError "DSS: Missing message is not in flight!"}
	       {Loop Tk Tm Last} %Should never happen!
	    end
	 end
      end
   in
      {Loop {Sort {Dictionary.keys Dic} Value.'<'} Miss Last}
   end
   class SiteMessenger
      attr
	 localStatus: 'StatusClean'
	 remoteStatus: 'StatusClean'
	 knownLocalStatus: 'StatusClean'
	 payloadToSend
	 messageToSend
	 remoteMonotonic: 0
	 lastRetransmit: 0

	 inFlight
	 missing
	 lastReceived:-1
      meth init()
	 @payloadToSend={New BoundedBuffer init(100)}
	 @messageToSend={New BoundedBuffer init(100)}
	 @inFlight={Dictionary.new}
	 @missing={Dictionary.new}
      end
      meth send(Payload)
	 {@payloadToSend put(Payload)}
	 if @localStatus == 'StatusClean' then
	    {self NewStatus('StatusInit')}
	 end
      end
      meth receive(Message)
	 if Message.senderMonotonic > remoteMonotonic then
	    remoteMonotonic := Message.senderMonotonic
	    knownLocalStatus := Message.receiverStatus
	    remoteStatus := Message.senderStatus
	    
	    {Acks @inFlight Message.missing Message.lastSeen}
	    for P in Message.payloads do
	       if P.id == @lastReceived+1 then
		  {@upper receive(P.pickle)}
	       elseif P.id<@lastReceived andthen {HasFeature @missing P.id} then
		  {Dictionary.remove @missing P.id}
		  {@upper receive(P.pickle)}
	       elseif P.id>@lastReceived+1 then
		  for I in @lastReceived+1..P.id-1 do @missing.I:=unit end
		  {@upper receive(P.pickle)}
	       end
	    end
	    
	    if @localStatus == @knownLocalStatus andthen
	       @localStatus == @remoteStatus then
	       %coherent state
	       case @localStatus
	       of 'StatusClean' andthen {@payloadToSend isNonEmpty($)} then
		  {self NewStatus('StatusInit')}
	       [] 'StatusClean' then
		  {self DropChannels()}
	       [] 'StatusInit' andthen {@payloadToSend isNonEmpty($)} then
		  {self NewStatus('StatusWorking')}
	       [] 'StatusInit' then
		  skip
	       [] 'StatusWorking' andthen {@payloadToSend isNonEmpty($)} then
		  {self ForceMessage()}
	       [] 'StatusWorking' then
		  skip
	       [] 'StatusClosing' then
		  skip
	       [] 'StatusClosed' andthen {@payloadToSend isNonEmpty($)} then
		  {self NewStatus('StatusInit')}
	       [] 'StatusClosed' then
		  {self NewStatus('StatusClean')}
	       [] 'StatusBroken' then
		  {self DropChannels()}
	       end
	    elseif @knownLocalStatus \= @localStatus then
	       {self ForceMessage()}
	    elsecase @localStatus#@remoteStatus
	    of 'StatusClean'#'StatusInit' then
	       {self NewStatus(@remoteStatus)}
	    [] 'StatusInit'#'StatusWorking' then
	       {self NewStatus(@remoteStatus)}
	    [] 'StatusWorking'#'StatusClosing' then
	       {self NewStatus(@remoteStatus)}
	    [] 'StatusClosing'#'StatusClosed' then
	       {self ForceMessage()}
	    [] 'StatusClosed'#'StatusClean' andthen {@payloadToSend isNonEmpty($)} then
	       {self NewStatus('StatusInit')}
	    [] 'StatusClosed'#'StatusClean' then
	       {self NewStatus(@remoteStatus)}
	    [] 'StatusClosed'#'StatusInit' then
	       {self NewStatus(@remoteStatus)}

	    [] _#'StatusBroken' then
	       {self NewStatus(@remoteStatus)}
	    [] 'StatusBroken'#_ then
	       {self ForceMessage()}

	    [] 'StatusClosed'#'StatusClosing' then
	       {self ForceMessage()}
	    else
	       {self NewStatus('StatusBroken')}
	    end
	    if @localStatus\='StatusBroken' then
	       if {HasFeature Message missing} then
		  if Message.receiverMonotonic >= @lastRetransmit then
		     {self ForceRetransmit(Message.missing)}
		  else
		     {self ForceMessage()}
		  end
	       end
	    end
	 end
      end
      meth NewStatus(NewS)
	 OldS = localStatus := NewS in
	 if OldS!=NewS then
	    {self ForceMessage()}
	 end
      end
      meth ForceRetransmit()
      end
      meth ForceMessage()
	 {@messageToSend put('SiteMessage'(senderStatus:@localStatus
					   receiverStatus:@remoteStatus
					   senderMonotonic:{NextMonotonic}
					   receiverMonotonic:@remoteMonotonic
					   lastSeen:@lastReceived))}
      end
   end
   
end