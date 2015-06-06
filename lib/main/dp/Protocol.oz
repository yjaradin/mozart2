functor
import
   GNode at 'x-oz://boot/GNode'
   BPort at 'x-oz://boot/Port'
export
   Manager
   RegisterBaseProtocols
define
   class Manager
      attr
	 localSite
	 protocols
	 objects
      meth init(LocalSite)
	 localSite := LocalSite
	 protocols := {Dictionary.new}
	 objects := {Dictionary.new}
      end
      meth keyFromGN(GN $)
	 {VirtualString.toAtom {Coders.decode {GNode.getUUID GN} [littleEndian latin1]}}
      end
      meth makeGlobal(RemoteSite GN Type ?ProtocolData)
	 ProtocolObject in
	 case {Dictionary.condExchange @objects {self keyFromGN(GN $)} unit $ ProtocolObject}
	 of unit then
	    ProtoDescr = {GNode.getProto GN} in
	    ProtocolObject = {New @protocols.({Label ProtoDescr}) initGlobal(@localSite ProtoDescr Type GN)}
	 [] E then
	    ProtocolObject = E
	 end
	 {ProtocolObject serialize(RemoteSite ProtocolData)}
      end
      meth makeProxy(RemoteSite GN Type ProtocolData)
	 ProtocolObject in
	 case {Dictionary.condExchange @objects {self keyFromGN(GN $)} unit $ ProtocolObject}
	 of unit then
	    ProtoDescr = {GNode.getProto GN} in
	    ProtocolObject = {New @protocols.({Label ProtoDescr}) initProxy(@localSite ProtoDescr Type GN RemoteSite ProtocolData)}
	 [] E then
	    ProtocolObject = E
	 end
	 {ProtocolObject waitInitialized()}
      end
      meth registerProtocol(Name Class)
	 @protocols.Name := Class
      end
   end

   class PortProtocol
      attr
	 originSite
	 dest
	 initialized:_
      meth initGlobal(LocalSite ProtoDescr Type GN)
	 P = {GNode.getValue GN} in
	 true = ProtoDescr == port
	 true = Type == port
	 originSite := LocalSite
	 dest := {NewName}
	 {LocalSite registerDispatchingTarget(@dest proc{$ RemoteSite portSend(to:Dest msg:M)}
						       {Send P M}
						       {RemoteSite sendMessage(portAck(to:Dest))}
						    end _)}
	 @initialized=unit
      end
      meth initProxy(LocalSite ProtoDescr Type GN RemoteSite ProtocolData)
	 OriginSite#Dest = ProtocolData
	 Sync = {NewCell unit} in
	 true = ProtoDescr == port
	 true = Type == port
	 originSite := {LocalSite getRemote(OriginSite RemoteSite $)}
	 dest := Dest
	 {LocalSite registerDispatchingTarget(Dest proc{$ _ portAck(to:_)}
						       @Sync=unit
						    end _)}
	 {GNode.getValue GN}={BPort.newWithGNode GN thread for M in $ do
							      Sync := _
							      {@originSite sendMessage(portSend(to:Dest msg:M))}
							      {Wait @Sync}
							   end end}
	 @initialized=unit
      end
      meth serialize(RemoteSite ?ProtocolData)
	 ProtocolData = {@originSite getId($)}#@dest
      end
      meth waitInitialized()
	 {Wait @initialized}
      end
   end
   proc{RegisterBaseProtocols Manager}
      {Manager registerProtocol(port PortProtocol)}
   end
end