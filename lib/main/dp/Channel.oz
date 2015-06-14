functor
import
   URL
   OS
   Open
define
   class Channel
      meth init(Url error:?Err)
      end
      meth send(Vbs error:?Err)
      end
      meth receive(?Vbs error:?Err)
      end
      meth close()
      end
   end
   class TcpOpenChannel
      attr
	 s
	 bad:false
      meth init(Url error:?Err)
	 try
	    U = {URL.make Url}
	    Auth = if U.authority == unit then localhost:9000 else U.authority end
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
   end
   class TcpOsChannel
      attr
	 conn
	 bad
      meth init(Url error:?Err)
	 try
	    U = {URL.make Url}
	    Auth = if U.authority == unit then localhost:9000 else U.authority end
	    Host P2 Port in
	    {List.takeDropWhile Auth fun{$C} C\=&: end Host P2}
	    if P2 \= nil then
	       Port = {StringToInt P2.2}
	    end
	    @conn = {OS.tcpConnect Host Port}
	 catch _ then
	    bad := true
	 end
	 Err = @bad
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
	    if {OS.tcpConnectionReadCompact Size Vbs $} \= Size then
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
end