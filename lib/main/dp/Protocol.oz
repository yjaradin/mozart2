functor
import
   GNode at 'x-oz://boot/GNode'
   BPort at 'x-oz://boot/Port'
   DPUtils(fifo:FIFO newFaultStream:NewFaultStream)
   System(postmortem:Postmortem printError:PrintError)
export
   Manager
   RegisterBaseProtocols
define
   class Manager
      attr
	 localSite
	 protocols
	 cleanupProxies
	 proxies
	 cleanupCoordinators
	 coordinators
      meth init(LocalSite)
	 @localSite = LocalSite
	 @protocols = {Dictionary.new}
	 @cleanupProxies = {Dictionary.new}
	 @proxies = {WeakDictionary.new thread for K#_ in $ do
						  {{Dictionary.condGet @cleanupProxies K proc{$}skip end}}
						  {Dictionary.remove @cleanupProxies K}
					       end end}
	 @cleanupCoordinators = {Dictionary.new}
	 @coordinators = {WeakDictionary.new thread for K#_ in $ do
							{{Dictionary.condGet @cleanupCoordinators K proc{$}skip end}}
							{Dictionary.remove @cleanupCoordinators K}
						     end end}
      end
      meth keyFromGN(GN $)
	 {VirtualString.toAtom {Coders.decode {GNode.getUUID GN} [littleEndian latin1]}}
      end
      meth makeProtocolObject(GN Type ?ProtocolObject)
	 case {WeakDictionary.condExchange @proxies {self keyFromGN(GN $)} unit $ ProtocolObject}
	 of unit then
	    ProtoDescr = {GNode.getProto GN}
	    C in
	    @cleanupProxies.{self keyFromGN(GN $)}:=C
	    ProtocolObject = {@protocols.({Label ProtoDescr}) initGlobal(@localSite ProtoDescr Type GN self C)}
	 [] E then
	    ProtocolObject = E
	 end
      end
      meth makeGlobal(RemoteSite GN Type ?ProtocolData)
	 {{self makeProtocolObject(GN Type $)} serialize(RemoteSite ProtocolData)}
      end
      meth makeProxy(RemoteSite GN Type ProtocolData)
	 ProtocolObject in
	 case {WeakDictionary.condExchange @proxies {self keyFromGN(GN $)} unit $ ProtocolObject}
	 of unit then
	    ProtoDescr = {GNode.getProto GN}
	    C in
	    @cleanupProxies.{self keyFromGN(GN $)}:=C
	    ProtocolObject = {@protocols.({Label ProtoDescr}) initProxy(@localSite ProtoDescr Type RemoteSite ProtocolData C)}
	 [] E then
	    ProtocolObject = E
	 end
	 {ProtocolObject bindTo(GN)}
      end
      meth registerCoordinator(Coordinator Cleanup)
	 N={NewName} in
	 @cleanupCoordinators.N:=Cleanup
	 {WeakDictionary.put @coordinators N Coordinator}
      end
      meth registerProtocol(Name ProxyMaker)
	 @protocols.Name := ProxyMaker
      end
   end

   ProxyKeepAlives = {Dictionary.new}
   local
      class PortProxy
	 attr
	    selfP
	    selfPO
	    status:'Unregistered'

	    localSiteId
	    localSite
	    coordinatorSiteId
	    coordinatorSite
	    coordinatorDest
	    holderData

	    messagesToSend
	    seq:~1

	    holding
	    
	    key:unit
	    targetCleaner:unit

	    faultStatus:ok
	    faultStream
	    faultStreamControl
	    coordinatorControl
	    sync:unit

	 meth preInit(SelfP SelfPO)
	    @selfP = SelfP
	    @selfPO = SelfPO
	    @holding = {Dictionary.new}
	    @messagesToSend = {New FIFO init()}
	    @faultStream = {NewFaultStream @faultStatus}
	 end
	 meth initGlobal(LocalSite ProtoDescr Type GN Manager ?Cleanup)
	    LocalPort
	    Coordinator
	    CoordinatorCleanup
	    N
	 in
	    @localSite = LocalSite
	    @localSiteId = {LocalSite getId($)}
	    {BPort.zombify {GNode.getValue GN} @selfP LocalPort}
	    Coordinator = {New PortCoordinator init(LocalSite LocalPort @coordinatorDest ?CoordinatorCleanup)}
	    @holderData = 'local'(Coordinator)
	    @coordinatorSite=LocalSite
	    @coordinatorSiteId={LocalSite getId($)}
	    {self WatchCoordinator()}
	    {Manager registerCoordinator(Coordinator CoordinatorCleanup)}
	    {self RegisterWithCoordinator()}
	    proc{Cleanup}
	       skip
	    end
	    N = {NewName}
	    @holding.N := unit
	    {Postmortem {GNode.getValue GN} @selfP Release(N)}
	 end
	 meth initProxy(LocalSite ProtoDescr Type RemoteSite ProtocolData ?Cleanup)
	    HolderData
	 in
	    @localSite = LocalSite
	    @localSiteId = {LocalSite getId($)}
	    @coordinatorSiteId#@coordinatorDest#HolderData = ProtocolData
	    @coordinatorSite={LocalSite getSite(@coordinatorSiteId RemoteSite $)}
	    @holderData={LocalSite getSite(HolderData.1 RemoteSite $)}#HolderData.2#HolderData.3
	    {self WatchCoordinator()}
	    {self RegisterWithCoordinator()}
	    proc{Cleanup}
	       skip
	    end
	 end
	 meth bindTo(GN)
	    if {BPort.newInGNode GN @selfP} then
	       N = {NewName} in
	       @holding.N := unit
	       {Postmortem {GNode.getValue GN} @selfP Release(N)}
	    end
	    {self SendMessages()}
	 end
	 meth serialize(RemoteSite ?ProtocolData)
	    ProtocolData=
	    case @status
	    of 'Broken' then
	       @coordinatorSiteId#@coordinatorDest#unit %Good luck
	    else
	       N={NewName}
	       Killer
	    in
	       @holding.N := Killer#RemoteSite
	       thread {WaitOr Killer {RemoteSite getBrokenAlarm($)}} {@selfPO Release(N)} end
	       @coordinatorSiteId#@coordinatorDest#(@localSiteId#@key#N)
	    end
	 end

	 meth getCoordinatorId($)
	    @coordinatorSiteId
	 end
	 meth WatchCoordinator()
	    thread if {Record.waitOr @coordinatorControl#{@coordinatorSite getBrokenAlarm($)}}==2 then {@selfPO Break(true)} end end
	    thread
	       FS = {@coordinatorSite getFaultStream($)}
	       proc{Loop FS}
		  case {Record.waitOr @faultStreamControl#FS}
		  of 1 then
		     {@faultStream.tell @faultStatus}
		     @sync=unit
		  [] 2 then
		     case FS.1
		     of ok then 
			{@faultStream.tell FS.1}
			@sync=unit
			{Loop FS.2}
		     [] tempFail(forever ...) then
			{@faultStream.tell FS.1}
			@sync=unit
			{Loop FS.2}
		     [] tempFail(...) then
			{@faultStream.tell FS.1}
			unit=sync:=_
			{Loop FS.2}
		     else
			{@faultStream.tell FS.1}
			@sync=unit
		     end
		  end
	       end
	    in
	       {Loop FS}
	    end
	 end
	 meth Break(DeadCoord)
	    if (@status=='Registering' andthen {Not DeadCoord})
	       orelse {Not {Dictionary.isEmpty @holding}} then
	       status:='Breaking'
	    else
	       status:='Broken'
	       {self unregisteredProxy()}
	       {self ReleaseHolder()}
	       {Dictionary.removeAll @holding}
	       {@messagesToSend getAll(_)}
	       @coordinatorControl=unit
	       coordinatorSite := unit
	    end
	 end
	 meth break()
	    faultStatus:=localFail(entity)
	    @faultStreamControl=unit
	    {self Break(false)}
	 end
	 meth kill()
	    case @status
	    of 'Broken' then skip
	    else
	       {@coordinatorSite sendMessage(kill(to:@coordinatoDest))}
	    end
	 end
	 meth getFaultStream($)
	    {@faultStream.get}
	 end

	 meth RegisterWithCoordinator()
	    if @status=='Unregistered' then
	       status := 'Registering'
	       ProxyKeepAlives.@coordinatorDest := @selfPO
	       key := {NewName}
	       seq := 0
	       targetCleaner := {@localSite registerDispatchingTarget(@key proc{$ RemoteSite Msg}
									      {@selfPO {AdjoinAt Msg remoteSite RemoteSite}}
									   end $)}
	       {@coordinatorSite sendMessage(registerProxy(to:@coordinatorDest errorTo:@key @key))}
	    end
	 end
	 meth UnregisterWithCoordinator()
	    if @status=='Registered' then
	       status := 'Unregistering'
	       {@coordinatorSite sendMessage(unregisterProxy(to:@coordinatorDest @key @seq))}
	       seq := ~1
	    end
	 end
	 meth registeredProxy(...)
	    case status:='Registered'
	    of 'Registering' then skip
	    [] 'Broken' then
	       status:='Broken'
	    [] 'Breaking' then
	       {self UnregisterWithCoordinator()}
	       {self Break(false)}
	    end
	    {self ReleaseHolder()}
	    {self SendMessages()}
	 end
	 meth ReleaseHolder()
	    case holderData := unit
	    of unit then skip
	    [] 'local'(_) then skip
	    [] GiverSite#GiverDest#GiverKey then
	       {GiverSite sendMessage(release(to:GiverDest GiverKey))}
	    end
	 end
	 meth unregisteredProxy(...)
	    case status:='Unregistered'
	    of 'Unregistering' then skip
	    [] 'Broken' then
	       status := 'Broken'
	    [] 'Breaking' then
	       {self Break(false)}
	    end
	    {Dictionary.remove ProxyKeepAlives @coordinatorDest}
	    {@targetCleaner}
	    key := unit
	    {self SendMessages()}
	 end
	 meth killed(...)
	    faultStatus:=permFail(entity)
	    @faultStreamControl=unit
	    {Dictionary.removeAll @holding}
	    {self Break(true)}
	 end
	 meth unknownTarget(...)
	    {self killed()}
	 end

	 meth portSend(Msg Sync)
	    Sync2=@sync in
	    case @status
	    of 'Broken' then skip
	    [] 'Breaking' then skip
	    else
	       {@messagesToSend put(Msg)}
	       {self SendMessages()}
	    end
	    thread
	       %{System.show 'waiting for sync'}
	       {Wait {self syncLocal($)}}
	       %{System.show 'got sync'}
	       Sync=Sync2
	    end
	 end
	 meth portSendRecv(Msg Res Sync)
	    Sync2=@sync in
	    {PrintError "Port.sendRecv is not supported on distributed ports."}
	    thread
	       {Wait {self syncLocal($)}}
	       Sync=Sync2
	    end
	 end

	 meth release(K ...)
	    {Dictionary.condGet @holding K unit#unit}=unit#_
	 end
	 meth Release(N)
	    {Dictionary.remove @holding N}
	    {self SendMessages()}
	 end
	 
	 meth SendMessages()
	    if {@messagesToSend isNonEmpty($)} then
	       case @status
	       of 'Registered' then
		  for Msg in {@messagesToSend getAll($)} do
		     {@coordinatorSite sendMessage(send(to:@coordinatorDest @key @seq Msg))}
		     seq := @seq + 1
		  end
	       [] 'Unregistered' then
		  {self RegisterWithCoordinator()}
	       else
		  skip
	       end
	    elseif {Dictionary.isEmpty @holding} then
	       {self UnregisterWithCoordinator()}
	    else
	       {self RegisterWithCoordinator()}
	    end
	 end
	 meth syncLocal($)
	    case @coordinatorSite
	    of unit then unit
	    [] S then
	       %{System.show 'coordinating on'(S)}
	       {S sync($)}
	    end
	 end
	 meth synchro($)
	    unit
	 end
      end
   in
      fun {NewPortProxy Init}
	 proc{PO M}
	    {Send P M}
	    {Wait{Send P synchro($)}}
	 end
	 P = {NewPort thread
			 Obj = {New PortProxy preInit(P PO)}
		      in
			 {Obj Init}
			 for M in $ do
			    {Obj M}
			 end
		      end}
      in
	 PO
      end
   end
   
   CoordinatorKeepAlives = {Dictionary.new}
   fun{NewSequencer}
      O={New class
		attr
		   status:alive
		   seqs
		   ooos
		meth init()
		   @seqs={Dictionary.new}
		   @ooos={Dictionary.new}
		end
		meth register(R K $)
		   if @status == alive then
		      @seqs.K:=0
		      @ooos.K:=nil
		      [register(R K)]
		   else
		      [kill(K)]
		   end
		end
		meth unregister(K)
		   {Dictionary.remove @seqs K}
		   {Dictionary.remove @ooos K}
		end
		meth kill(?Res)
		   status:=dead
		   Res = {Map {Dictionary.keys @seq}
			  fun{$ K}
			     kill(K)
			  end}
		   {Dictionary.removeAll @seqs}
		   {Dictionary.removeAll @ooos}
		end
		meth sequence(K S M $)
		   if {HasFeature @seqs K} then
		      if @seqs.K == S then
			 Good Bad in
			 @seqs.K := S + 1
			 {List.takeDropWhile @ooos.K fun{$ P}
							if P.1==@seqs.K then
							   @seqs.K := P.1+1
							   true
							else
							   false
							end
						     end Good Bad}
			 @ooos.K := Bad
			 M|{Map Good fun{$ P}P.2 end}
		      else
			 @ooos.K := {List.merge @ooos.K [S#M] fun{$ P1 P2}P1.1<P2.1 end}
			 nil
		      end
		   else
		      nil
		   end
		end
	     end init()}
      P={NewPort thread
		    for M in $ do
		       {O M}
		    end
		 end}
   in
      proc{$ M}
	 {Send P M}
      end
   end
   class PortCoordinator
      attr
	 status:alive
	 localPort
	 sequencer
      meth init(LocalSite LocalPort ?CoordinatorDest ?Cleanup)
	 Dest = {NewName} in
	 @localPort = LocalPort
	 @sequencer = {NewSequencer}
	 {LocalSite registerDispatchingTarget(Dest proc{$ RemoteSite Msg}
						      {self {AdjoinAt Msg remoteSite RemoteSite}}
						   end Cleanup)}
	 CoordinatorDest = Dest
      end

      meth registerProxy(ProxyKey remoteSite:RemoteSite ...)
	 Killer Done in
	 thread
	    {Value.waitOr Killer {RemoteSite getBrokenAlarm($)}}
	    {self InternalUnregister(ProxyKey Done)}
	 end
	 CoordinatorKeepAlives.ProxyKey:=Killer#Done#RemoteSite
	 {self ProcessSeq({@sequencer register(RemoteSite ProxyKey $)})}
      end

      meth unregisterProxy(ProxyKey Seq remoteSite:RemoteSite ...)
	 {self ProcessSeq({@sequencer sequence(ProxyKey Seq unregister(RemoteSite ProxyKey) $)})}
      end

      meth InternalUnregister(ProxyKey ?Done)
	 {Dictionary.remove CoordinatorKeepAlives ProxyKey}
	 {@sequencer unregister(ProxyKey)}
	 Done=unit
      end

      meth send(ProxyKey Seq Msg remoteSite:RemoteSite ...)
	 {self ProcessSeq({@sequencer sequence(ProxyKey Seq send(Msg) $)})}
      end

      meth ProcessSeq(L)
	 for M in L do
	    case M
	    of unregister(RemoteSite ProxyKey) then
	       Done in
	       {Dictionary.condGet CoordinatorKeepAlives ProxyKey unit#unit#unit}=unit#Done#_
	       {Wait Done}
	       {RemoteSite sendMessage(unregisteredProxy(to:ProxyKey))}
	    [] send(Msg) then
	       {Send @localPort Msg}
	    [] register(RemoteSite ProxyKey) then
	       {RemoteSite sendMessage(registeredProxy(to:ProxyKey))}
	    [] kill(ProxyKey) then
	       case {Dictionary.condGet CoordinatorKeepAlives ProxyKey unit}
	       of unit then skip
	       [] Killer#_#RemoteSite then
		  Killer=unit
		  {RemoteSite sendMessage(killed(to:ProxyKey))}
	       end
	    end
	 end
      end

      meth kill(remoteSite:RemoteSite ...)
	 status:=dead
	 localPort:={NewPort _}
	 {self ProcessSeq({@sequencer kill($)})}
      end

   end
   
   proc{RegisterBaseProtocols Manager}
      {Manager registerProtocol(port NewPortProxy)}
   end
end