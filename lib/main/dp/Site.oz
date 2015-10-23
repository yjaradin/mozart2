functor
import
   Protocol
   Serialization(serializer:Serializer deserializer:Deserializer)
   Channel
   PBuf at 'x-oz://boot/Protobuf'
   BWeakRef at 'x-oz://boot/WeakReference'
   BName at 'x-oz://boot/Name'
   BTime at 'x-oz://boot/Time'
   System
   DPUtils(fifo:FIFO newFaultStream:NewFaultStream)
   VM
   DPVM
export
   This
   Is
   remotes:ListRemotes
   OnNewRemote
define
   SitesByUrl = {WeakDictionary.new _}
   {WeakDictionary.close SitesByUrl}
   proc{UpdateUrls S Old New}
      case Old#New
      of _#nil then
	 for U in Old do {WeakDictionary.remove SitesByUrl U} end
      [] nil#_ then
	 for U in New do {WeakDictionary.put SitesByUrl U S} end
      [] (O|Or)#(N|Nr) then
	 if O<N then
	    {WeakDictionary.remove SitesByUrl O}
	    {UpdateUrls S Or New}
	 elseif O>N then
	    {WeakDictionary.put SitesByUrl N S}
	    {UpdateUrls S Old Nr}
	 else
	    {UpdateUrls S Or Nr}
	 end
      end
   end

   SiteMarker = {NewName}
   class Site
      feat
	 !SiteMarker: unit
      attr
	 shortId
	 reachability
	 reachabilitySync
      meth init()
	 @reachability = r(version:~1 addresses:nil)
      end
      meth getId($)
	 @shortId
      end
      meth getIdBytes($)
	 {VirtualByteString.fromBase64 {BName.uniqueToAtom @shortId}}
      end
      meth idFromBytes(Bs $)
	 {BName.newUnique {VirtualString.toAtom {VirtualByteString.toBase64 Bs}}}
      end
      meth setReachability(R)
	 if R.version > @reachability.version then
	    Old = reachability := R in
	    {UpdateUrls self
	     {Sort {Map Old.addresses fun{$ A}A.url end} Value.'<'}
	     {Sort {Map R.addresses   fun{$ A}A.url end} Value.'<'}}
	    @reachabilitySync=unit
	 end
      end
      meth getReachability($)
	 @reachability
      end
      meth reachabilitySync($)
	 @reachabilitySync
      end
      meth break()
	 {self localFail(break)}
      end
      meth kill()
	 {self sendMessage(siteKill(to:site))}
	 thread {Delay 20000} {self break()} end
      end
   end
   fun{Is S}
      {HasFeature S SiteMarker}
   end

   proc{DefaultDispatch FromSite Msg}
      if {HasFeature Msg errorTo} then
	 {FromSite sendMessage(unknownTarget(to:Msg.errorTo Msg.to))}
      end
   end
   class ThisSite
      attr
	 protocolManager
	 dispatcher
      from Site
      meth init()
	 Site,init()
	 {self setReachability(r(version:0 addresses:nil))}
	 @shortId = {self idFromBytes({VirtualByteString.newUUID} $)}
	 protocolManager := {New Protocol.manager init(self)}
	 {Protocol.registerBaseProtocols @protocolManager}
	 dispatcher := {Dictionary.new}
	 {self registerDispatchingTarget(site proc{$ From Msg}
						 case Msg
						 of siteEnquire(to:_ ask:ShortId) then
						    S = {self getSite(ShortId unit $)} in
						    %{System.show enquire(ShortId)}
						    {Wait {S reachabilitySync($)}}
						    %{System.show synced(ShortId)}
						    {From sendMessage(siteAnswer(to:site
										 id:ShortId
										 descr:{S getReachability($)}))}
						 [] siteAnswer(to:_ id:ShortId descr:Descr) then
						    {{self getRemote(ShortId unit $)} setReachability(Descr)}
						 [] siteKill(to:_) then
						    {System.showError 'DP: killed by '#{BName.uniqueToAtom {From getId($)}}}
						    {VM.kill {VM.current}}
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
      meth getSite(ShortId KnowingSite $)
	 if ShortId==@shortId then
	    self
	 else
	    {self getRemote(ShortId KnowingSite $)}
	 end
      end
      meth getRemote(ShortId KnowingSite $)
	 true = ShortId\=@shortId
	 Remote = {GetRemoteSite ShortId} in
	 if {Remote getReachability($)}.version == ~1 then
	    RemChan = {{Remote getMessenger($)} getChannel($)}
	 in
	    if RemChan\= unit andthen {Not {RemChan closed($)}} then
	       {Remote sendMessage(siteEnquire(to:site ask:ShortId))}
	    end
	    if KnowingSite\=unit then
	       {KnowingSite sendMessage(siteEnquire(to:site ask:ShortId))}
	    end
	 end
	 Remote
      end
      meth getRemoteByUrl(Url $) N in
	 case {WeakDictionary.condExchange SitesByUrl Url unit $ N}
	 of unit then
	    C = {New HighLevelChannel init(url:Url)} in
	    if {C closed($)} then
	       N = unit
	    else
	       N = {C getRemote($)}
	    end
	 [] S then
	    N=S
	 end
	 N
      end
      meth offerChannel(Chan)
	 {New HighLevelChannel init(chan:Chan) _}
      end
      meth sendMessage(Msg)
	 thread
	    {self dispatchMessage(self Msg)}
	 end
      end
      meth dispatchMessage(FromSite Msg)
	 if {Record.is Msg} andthen
	    {HasFeature Msg to} then
	    {{Dictionary.condGet @dispatcher Msg.to DefaultDispatch} FromSite Msg}
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
      meth getFaultStream($)
	 ok|_
      end
      meth getBrokenAlarm($)
	 _
      end
   end
   This = {New ThisSite init()}

   local
      class RemoteSite
	 from Site
	 attr
	    localSite
	    messenger
	 meth init(ShortId)
	    Site,init()
	    @localSite = This
	    @shortId = ShortId
	    @messenger = {GetMessenger ShortId self}
	 end

	 meth setReachability(R)
	    Site,setReachability(R)
	    {@messenger setAddresses(R.addresses)}
	 end

	 meth getMessenger($)
	    @messenger
	 end

	 meth receive(VBS)
	    thread
	       {@localSite dispatchMessage(self {{New Deserializer init(@localSite self VBS)} close($)})}
	    end
	 end

	 meth sendMessage(Msg)
	    thread
	       {@messenger send({{New Serializer init(@localSite self Msg)} close($)})}
	    end
	 end

	 meth localFail(...)=M
	    {@messenger M}
	 end

	 meth permFail(...)=M
	    {@messenger M}
	 end

	 meth getFaultStream($)
	    {@messenger getFaultStream($)}
	 end

	 meth getBrokenAlarm($)
	    {@messenger getBrokenAlarm($)}
	 end
       end
      Remotes={WeakDictionary.new _}
   in
      {WeakDictionary.close Remotes}
      OnNewRemote={Dictionary.new}
      fun{GetRemoteSite ShortId}
	 N in
	 case {WeakDictionary.condExchange Remotes ShortId unit $ N}
	 of unit then
	    N={New RemoteSite init(ShortId)}
	    for P in {Dictionary.items OnNewRemote} do
	       thread
		  {P N}
	       end
	    end
	 [] Rem then
	    N=Rem
	 end
	 N
      end
      fun{ListRemotes}
	 {WeakDictionary.items Remotes}
      end
   end





   
   fun{GetProtoInt Vbs Idx V S ?LIdx}
      B = {VirtualByteString.get Vbs Idx} in
      if B<128 then
	 LIdx=Idx
	 V+B*S
      else
	 {GetProtoInt Vbs Idx+1 V+(B-128)*S S*128 LIdx}
      end
   end
   Magic = [&D &s &s 0xDE 0xC0 0xDE &P &B &u &f 0xF0 0x0D &v &1 &. &0]
   MagicLength = 16
   MsgKind = {VirtualString.toCompactString "mozart.pb.Stream"}
   PBufPool = {PBuf.newPool}
   class HighLevelChannel
      attr
	 chan
	 closed: false
	 upper
	 needUpper:true
	 remoteId
      meth init(url:Url<=unit chan:Chan<=unit)
	 if Url \= unit then
	    if {New {Channel.urlToChannelClass Url} init(Url error:$) @chan} then {self close} end
	 else
	    true = Chan \= unit
	    @chan = Chan
	 end
	 thread
	    if {@chan send(Magic error:$)} then {self close} end
	    if {VirtualByteString.toList {self receiveBytes(MagicLength $)}} \= Magic then {self close} end
	    {self sendChannel('ChannelMessage'(senderSiteId:{This getIdBytes($)}))}
	    for until:{self closed($)} do
	       H T Header in
	       {self receiveBytes(6 Header)}
	       if {VirtualByteString.get Header 0}\=10 then {self close()} end
	       H = {GetProtoInt Header 1 0 1 T}
	       {self receivedBytes(Header#{self receiveBytes(T+H-5 $)})}
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
	 if {@chan receive(Vbs size:Size error:$)} then {self close} end
      end
      meth receivedBytes(Vbs)
	 if @closed then skip else
	    {self received({PBuf.bytesToRecord PBufPool MsgKind Vbs}.elements.1)}
	 end
      end
      meth getRemote($)
	 {Wait @remoteId}
	 {This getRemote(@remoteId unit $)}
      end
      meth received(ChannelMsg)
	 if {HasFeature ChannelMsg senderSiteId} andthen @needUpper then
	    @remoteId={This idFromBytes(ChannelMsg.senderSiteId $)}
	    @upper={{self getRemote($)} getMessenger($)}
	    {@upper offerChannel(self)}
	    needUpper := false
	 end
	 if {HasFeature ChannelMsg siteMessage} then
	    {@upper receive(ChannelMsg.siteMessage)}
	 end
      end
      meth send(SiteMsg)
	 {self sendChannel('ChannelMessage'(siteMessage:SiteMsg))}
      end
      meth sendChannel(ChannelMsg)
	 Bs={PBuf.recordToBytes PBufPool MsgKind 'Stream'(elements:[ChannelMsg])} in
	 if {@chan send(Bs error:$)} then {self close} end
      end
   end

   local
      local
	 Ctr={NewCell 0}
      in
	 fun {NextMonotonic}
	    N O = Ctr := N in
	    N=O+1
	    O
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
		  {System.showError "DSS: Missing message is not yet in flight!"}
		  {Loop Tk Tm Last} %Should never happen!
	       end
	    end
	 end
      in
	 {Loop {Sort {Dictionary.keys Dic} Value.'<'} Miss Last}
      end
      KeptBroken={Dictionary.new}
      class SiteMessenger
	 attr
	    key
	    
	    selfPort
	    upper
	    upperIsGCed
	    channel: unit
	    addresses: nil
	    channelRequest: false
	    
	    localStatus: 'StatusClean'
	    remoteStatus: 'StatusClean'
	    knownLocalStatus: 'StatusClean'
	    payloadToSend
	    remoteMonotonic: 0
	    lastRetransmit: 0

	    forceMessage: false
	    toRetransmit: nil

	    inFlight
	    missing
	    remoteNext:0
	    localNext:0

	    dropChannelsCountdown:0

	    remoteTime:0
	    lastReceivedTime:0
	    lastSentTime:0
	    rttPredictor

	    periodicThreadControl
	    failureStatus: tempFail(notNeeded)
	    faultStream
	    brokenAlarm
	    
	 meth init(upper:Upper selfPort:P)
	    @key={NewName}
	    @selfPort=P
	    {self setUpper(Upper)}
	    @payloadToSend={New FIFO init()}
	    @inFlight={Dictionary.new}
	    @missing={Dictionary.new}
	    @rttPredictor={New Predictor init()}
	    @faultStream={NewFaultStream @failureStatus}
	    thread
	       proc{Loop}
		  {Wait @periodicThreadControl}
		  {Delay {Send @selfPort PeriodicCheck($)}}
		  {Loop}
	       end in
	       {Loop}
	    end
	 end
	 meth setAddresses(As)
	    addresses := As
	    {self SendIfNeeded()}
	 end
	 meth offerChannel(Chan)
	    if {Not {Chan closed($)}} then
	       OldChan = channel := Chan in
	       if OldChan \= Chan andthen OldChan \= unit then
		  {OldChan close()}
	       end
	       @periodicThreadControl=unit
	       {self SendIfNeeded()}
	    end
	 end
	 meth getChannel($)
	    @channel
	 end
	 meth setUpper(Upper)
	    local WR={BWeakRef.new Upper} in
	       upper := proc{$ M}
			   case {BWeakRef.get WR}
			   of some(U) then {U M}
			   else skip
			   end
			end
	       upperIsGCed := fun{$}
				 {BWeakRef.get WR}==none
			      end
	    end
	 end
	 meth send(Payload)
	    {@payloadToSend put(Payload)}
	    if @localStatus == 'StatusClean' then
	       {self NewStatus('StatusInit')}
	    end
	    {self SendIfNeeded()}
	 end
	 meth receive(Message)
	    if Message.senderMonotonic > @remoteMonotonic then
	       {self FailStatus(ok)}
	       remoteMonotonic := Message.senderMonotonic
	       lastReceivedTime := {@rttPredictor currentTime($)}
	       remoteTime := Message.senderTime
	       if Message.receiverTime\=0 then
		  {@rttPredictor ping2(Message.receiverTime+Message.senderDelay @lastReceivedTime)}
	       end
	       knownLocalStatus := Message.receiverStatus
	       remoteStatus := Message.senderStatus
	    
	       {Acks @inFlight {Value.condSelect Message missing nil} Message.receiverNext-1}
	       for I in @remoteNext..Message.senderNext-1 do
		  @missing.I:=unit
	       end
	       remoteNext := Message.senderNext
	       for P in {Value.condSelect Message payloads nil} do
		  if {HasFeature @missing P.id} then
		     {Dictionary.remove @missing P.id}
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
		     forceMessage := true
		  [] 'StatusWorking' andthen {@upperIsGCed} then
		     {self NewStatus('StatusClosing')}
		  [] 'StatusWorking' then
		     skip
		  [] 'StatusClosing' andthen {Dictionary.isEmpty @inFlight} andthen {Dictionary.isEmpty @missing} then
		     {self NewStatus('StatusClosed')}
		  [] 'StatusClosing' then
		     forceMessage := true
		  [] 'StatusClosed' andthen {@payloadToSend isNonEmpty($)} then
		     {self NewStatus('StatusInit')}
		  [] 'StatusClosed' then
		     {self NewStatus('StatusClean')}
		  [] 'StatusBroken' then
		     {self DropChannels()}
		  end
	       elseif @knownLocalStatus \= @localStatus then
		  forceMessage := true
	       elsecase @localStatus#@remoteStatus
	       of 'StatusClean'#'StatusInit' then
		  {self NewStatus(@remoteStatus)}
	       [] 'StatusInit'#'StatusWorking' then
		  {self NewStatus(@remoteStatus)}
	       [] 'StatusWorking'#'StatusClosing' then
		  {self NewStatus(@remoteStatus)}
	       [] 'StatusClosing'#'StatusClosed' andthen {Dictionary.isEmpty @inFlight} andthen {Dictionary.isEmpty @missing} then
		  {self NewStatus(@remoteStatus)}
	       [] 'StatusClosing'#'StatusClosed' then
		  forceMessage := true
	       [] 'StatusClosed'#'StatusClean' andthen {@payloadToSend isNonEmpty($)} then
		  {self NewStatus('StatusInit')}
	       [] 'StatusClosed'#'StatusClean' then
		  {self NewStatus(@remoteStatus)}
	       [] 'StatusClosed'#'StatusInit' then
		  {self NewStatus(@remoteStatus)}

	       [] _#'StatusBroken' then
		  {self NewStatus(@remoteStatus)}
	       [] 'StatusBroken'#_ then
		  forceMessage := true

	       [] 'StatusClosed'#'StatusClosing' then
		  forceMessage := true
	       else
		  {self NewStatus('StatusBroken')}
	       end
	       if @localStatus\='StatusBroken' then
		  if {HasFeature Message missing} then
		     forceMessage := true
		     if Message.receiverMonotonic >= @lastRetransmit then
			toRetransmit := Message.missing
		     end
		  end
	       end
	    end
	    {self SendIfNeeded()}
	 end
	 meth DropChannels()
	    if @dropChannelsCountdown == 0 then
	       dropChannelsCountdown := 10
	    end
	 end
	 meth NewStatus(NewS)
	    OldS = localStatus := NewS in
	    if OldS\=NewS then
	       forceMessage := true
	    end
	    case NewS
	    of 'StatusInit' then
	       {self FailStatus(tempFail(connecting))}
	       dropChannelsCountdown := 0
	       @periodicThreadControl=unit
	    [] 'StatusClean' then
	       {self FailStatus(tempFail(notNeeded))}
	    [] 'StatusBroken' then
	       @brokenAlarm = unit
	       KeptBroken.@key:=self
	       {self FailStatus(tempFail(forever))}
	    else
	       skip
	    end
	 end
	 meth SendIfNeeded()
	    if @channel\=unit andthen {Not {@channel closed($)}} then
	       ThisMonotonic = {NextMonotonic}
	       OldPayloads = {Map @toRetransmit fun{$ I} @inFlight.I end}
	       NewPayloads = if @localStatus == 'StatusWorking' then
				{Map {@payloadToSend getAll($)} fun{$ P}
								   'SitePayload'(id:localNext := @localNext + 1 pickle:P)
								end}
			     else
				nil
			     end
	    in
	       if @forceMessage orelse NewPayloads \= nil then
		  for P in NewPayloads do
		     @inFlight.(P.id):=P
		  end
		  lastSentTime:={@rttPredictor currentTime($)}
		  {@channel send('SiteMessage'(missing:{Sort {Dictionary.keys @missing} Value.'<'}
					       payloads:{Append OldPayloads NewPayloads}
					       senderStatus:@localStatus
					       receiverStatus:@remoteStatus
					       senderMonotonic:ThisMonotonic
					       receiverMonotonic:@remoteMonotonic
					       senderNext:@localNext
					       receiverNext:@remoteNext
					       senderTime:@lastSentTime
					       receiverTime:@remoteTime
					       senderDelay:@lastSentTime-@lastReceivedTime))}
		  forceMessage := false
		  if @toRetransmit \= nil then
		     toRetransmit := nil
		     lastRetransmit := ThisMonotonic
		  end
	       end
	    elseif {Not @channelRequest} andthen @localStatus \= 'StatusBroken' andthen (@forceMessage orelse {@payloadToSend isNonEmpty($)}) then
	       Addresses = @addresses in
	       channelRequest := true
	       thread
		  for A in {Sort Addresses fun{$ A1 A2} A1.quality>A2.quality end}
		     break:B do
		     C = {New HighLevelChannel init(url:A.url)} in
		     if {Not {C closed($)}} then
			{B}
		     end
		  end
		  {Send @selfPort ResetChannelRequest()}
	       end
	    end
	 end
	 meth ResetChannelRequest()
	    channelRequest := false
	 end
	 meth PeriodicCheck(?DelayBeforeNextCall)
	    C = {@rttPredictor currentTime($)}
	    Bounds = {@rttPredictor getBounds($)}
	 in
	    if C-@lastSentTime > 3*Bounds.min - Bounds.max then
	       forceMessage := true
	    end
	    if C-@lastReceivedTime > 5*Bounds.max then
	       {self FailStatus(tempFail(timeout))}
	    end
	    {self SendIfNeeded()}
	    if @dropChannelsCountdown==1 then
	       dropChannelsCountdown := 0
	       DelayBeforeNextCall = 0
	       periodicThreadControl := _
	       if @channel \= unit then
		  {@channel close()}
		  channel := unit
	       end
	    else
	       if @dropChannelsCountdown>0 then
		  dropChannelsCountdown := @dropChannelsCountdown - 1
	       else
		  if (@channel == unit orelse {@channel closed($)}) andthen @failureStatus \= ok then
		     periodicThreadControl := _
		  end
	       end
	       DelayBeforeNextCall = {Max 10 ({Min @lastSentTime+3*Bounds.min @lastReceivedTime+5*Bounds.max}
					      -{@rttPredictor currentTime($)})}
	    end
	    %{System.show DelayBeforeNextCall#Bounds#C-@lastReceivedTime}
	 end
	 meth FailStatus(Candidate)
	    proc{DoIt}
	       Old = failureStatus := Candidate in
	       if Old\=Candidate then
		  {@faultStream.tell Candidate}
	       end
	    end in
	    case @failureStatus#Candidate
	    of permFail(...)#_         then skip
	    [] _#permFail(...)         then {DoIt}
	    [] localFail(...)#_        then skip
	    [] _#localFail(...)        then {DoIt}
	    [] tempFail(forever)#_     then skip
	    [] _#tempFail(forever)     then {DoIt}
	    [] tempFail(notNeeded)
	       #tempFail(connecting)   then {DoIt}
	    [] tempFail(notNeeded)#_   then skip
	    [] _#ok                    then {DoIt}
	    [] _#tempFail(notNeeded)   then {DoIt}
	    [] tempFail(connecting)#_  then skip
	    [] _#tempFail(timeout)     then {DoIt}
	    [] tempFail(notNeeded)#_   then {DoIt}
	    [] _#tempFail(connecting)  then skip
	    end
	 end
	 meth localFail(...)=M
	    {self NewStatus('StatusBroken')}
	    {Dictionary.remove KeptBroken @key}
	    {self FailStatus(M)}
	 end
	 meth permFail(...)=M
	    {self NewStatus('StatusBroken')}
	    {Dictionary.remove KeptBroken @key}
	    {self FailStatus(M)}
	 end
	 meth getFaultStream($)
	    {@faultStream.get}
	 end
	 meth getBrokenAlarm($)
	    @brokenAlarm
	 end
      end
      Messengers={WeakDictionary.new _}
   in
      {WeakDictionary.close Messengers}
      fun{GetMessenger ShortId Upper}
	 N in
	 case {WeakDictionary.condExchange Messengers ShortId unit $ N}
	 of unit then
	    P={NewPort thread for M in $ do {O M} end end}
	    O={New SiteMessenger init(upper:Upper selfPort:P)}
	 in
	    N=proc{$ M}{Send P M}end
	 [] Msger then
	    N=Msger
	    {N setUpper(Upper)}
	 end
	 N
      end
   end

   fun{Clamp V Min Max}
      if V<Min then Min
      elseif V>Max then Max
      else V
      end
   end
   
   class Predictor
      attr
	 min
	 max
      meth init()
	 @min=50000 %50s
	 @max=10 %10ms
      end
      meth ping2(T1 T2)
	 D = {Clamp {Clamp T2-T1 @min div 2 @max * 2} 10 50000}
	 Delta = (@max-@min) div 10 in
	 min:={Min D @min+Delta}
	 max:={Max D @max-Delta}
      end
      meth currentTime($)
	 {BTime.getReferenceTime}
      end
      meth getBounds($)
	 if @min<@max then
	    bounds(min:@min max:@max)
	 else
	    bounds(min:@max max:@max)
	 end
      end
   end
   {DPVM.init This}
end