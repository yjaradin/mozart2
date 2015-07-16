functor
import
   VM(ncores:Ncores
      current:VMCurrent
      new:VMNew
      getPort:VMGetPort
      identForPort%unused
      getStream:GetStream
      closeStream:CloseStream
      list%unused
      kill:VMKill
      monitor%unused
     )
   Site
   DP
   Connection
export
   Ncores
   Current
   New
   GetPort
   IdentForPort
   GetStream
   CloseStream
   List
   Kill
   Monitor
   
   Init
define
   local
      Queries = {NewDictionary}
   in
      fun{KeyFor X}
	 K={NewName} in
	 Queries.K:=X
	 K
      end
      proc{KeyIs K X}
	 Queries.K=X
	 {Dictionary.remove Queries K}
      end
   end
   VMP={VMGetPort {VMCurrent}}
   [Answer AskCmd AskSiteCmd KillCmd] = {ForAll $ NewName}
   CurrentP = {NewPort thread
			  for M in $ do
			     case M
			     of Answer(K X) then {KeyIs K X}
			     [] AskCmd(K D) then {Send D Answer(K Current)}
			     [] AskSiteCmd(K D) then {Send D Answer(K Site.this)}
			     [] KillCmd() then {VMKill {VMCurrent}}
			     else {Send VMP M}
			     end
			  end
		       end}
   CurrentId = thread {VirtualString.toAtom {Connection.offerUnlimited CurrentP}} end
   fun{Current}
      {Wait CurrentId}
      CurrentId
   end
   proc{New App ?Res}
      {Wait CurrentId}
      Key = {KeyFor Res} in
      {VMNew functor
	     import
		DPVM
		Connection
		Module
		Property
	     define
		{Send {Connection.take CurrentId} Answer(Key {DPVM.current})}
		{Wait
		 if {IsAtom App} then
		    {Property.set 'application.url' App}
		    {Module.link [App]}.1
		 else
		    {Module.apply [App]}.1
		 end}
	     end _}
   end
   fun{GetPort Id}
      {Connection.take Id}
   end
   proc{IdentForPort Id ?Res}
      {Send {Connection.take Id} AskCmd({KeyFor Res} CurrentP)}
   end
   proc{Kill Id}
      {Send {Connection.take Id} KillCmd()}
   end
   proc{Monitor Id}
      Remote in
      {Send {Connection.take Id} AskSiteCmd({KeyFor Remote} CurrentP)}
      thread
	 for State in {DP.getFaultStream {Site.this getRemote(Remote unit $)}} do
	    case State
	    of permFail(...) then
	       {Send VMP terminated(Id reason:State)}
	    else skip
	    end
	 end
      end
   end
   fun{List}
      {Map {Site.remotes} proc{$ R ?Res}
			     {R sendMessage(dpvmGet(to:dpvm {KeyFor Res} CurrentP))}
			  end}
   end
   proc{Init ThisSite}
      {ThisSite registerDispatchingTarget(dpvm proc{$ From M}
						   case M
						   of dpvmGet(to:_ K P) then
						      {Wait CurrentId}
						      {Send P Answer(K CurrentId)}
						   end
						end _)}
   end
end