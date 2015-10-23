functor
import
   URL
   OS
   Property
   Site
export
   UrlToChannelClass
   ListenTcp
   Init
define
   /*class Channel
      meth init(Url error:?Err)
      end
      meth send(Vbs error:?Err)
      end
      meth receive(?Vbs error:?Err)
      end
      meth close()
      end
     end*/
   fun{UrlToChannelClass Url}
      case {URL.make Url}.scheme
      of "x-dss-tcp" then
	 TcpOsChannel
      end
   end
   /* Unused, TcpOsChannel should be equivalent & more efficient
   class TcpOpenChannel
      attr
	 s
	 bad:false
      meth init(Url error:?Err)
	 try
	    U = {URL.make Url}
	    Auth = if U.authority == unit then "localhost:9000" else U.authority end
	    Host P2 Port in
	    {List.takeDropWhile Auth fun{$C} C\=&: end Host P2}
	    if P2 \= nil then
	       Port = {StringToInt P2.2}
	    end
	    @s = {New Open.socket client(host:Host port:Port)}
	 catch _ then
	    bad := true
	 end
	 Err = @bad
      end
      meth send(Vbs error:?Err)
	 try
	    {@s write(vs:Vbs)}
	 catch _ then
	    bad := true
	 end
	 Err = @bad
      end
      meth receive(?Vbs size:Size error:?Err)
	 try
	    if {@s read(list:Vbs size:Size len:$)} \= Size then
	       bad := true
	    end
	 catch _ then
	    bad := true
	 end
	 Err = @bad
      end
      meth close()
	 try
	    {@s close()}
	 catch _ then
	    skip
	 finally
	    bad := true
	 end
      end
   end*/
   class TcpOsChannel
      attr
	 conn
	 bad:false
      meth init(Url error:?Err)
	 try
	    U = {URL.make Url}
	    Host Port in
	    {StringToInt {List.takeDropWhile U.authority fun{$C} C\=&: end ?Host}.2 ?Port}
	    @conn = {OS.tcpConnect Host Port}
	 catch _ then
	    bad := true
	 end
	 Err = @bad
      end
      meth initWithConn(Conn)
	 @conn=Conn
      end
      meth send(Vbs error:?Err)
	 try
	    {OS.tcpConnectionWrite @conn Vbs _}
	 catch _ then
	    bad := true
	 end
	 Err = @bad
      end
      meth receive(?Vbs size:Size error:?Err)
	 try
	    if {OS.tcpConnectionReadCompact @conn Size Vbs $} \= Size then
	       bad := true
	    end
	 catch _ then
	    bad := true
	 end
	 Err=@bad
      end
      meth close()
	 try
	    {OS.tcpConnectionClose @conn}
	 catch _ then
	    skip
	 finally
	    bad := true
	 end
      end
   end
   proc{ListenTcp}
      Uniquifier = {Map {List.takeWhile {VirtualString.toString {VirtualByteString.toBase64 {Site.this getIdBytes($)}}} fun{$ C}C\=&=end}
		    fun{$C}
		       case C
		       of &/ then &_
		       [] &+ then &-
		       else C
		       end
		    end}
      fun{FindPort}
	 try
	    Port = 9000+{OS.rand}mod 10000
	    Acc = {OS.tcpAcceptorCreate 4 Port}
	 in
	    Port#Acc
	 catch _ then
	    {FindPort}
	 end
      end
      Port#Acc = {FindPort}
      proc{ProcessIncoming}
	 {Site.this offerChannel({New TcpOsChannel initWithConn({OS.tcpAccept Acc})})}
	 {ProcessIncoming}
      end
   in
      {Site.this addAddress(a(url:{VirtualString.toAtom "x-dss-tcp://"#{Property.condGet 'dp.dss_tcp.host' localhost}#":"#Port#"/"#Uniquifier#"/"}
			      scope:{Property.condGet 'dp.dss_tcp.scope' 10}
			      quality:100))}
      thread
	 {ProcessIncoming}
      end
   end
   Uninitialized={NewCell true}
   proc{Init} X in
      if Uninitialized := X then
	 for P in {Property.condGet 'dp.defaultListeners' [ListenTcp]} do
	    {P}
	 end
	 X=false
      end
   end
end