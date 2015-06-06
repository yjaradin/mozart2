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
   
end