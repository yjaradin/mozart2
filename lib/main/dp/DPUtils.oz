functor
export
   fifo:FIFO
   NewFaultStream
define
   class FIFO
      attr
	 head
	 tail
      meth init()
	 @head=@tail
      end
      meth put(V) N in
	 V|N = tail := N
      end
      meth isNonEmpty($)
	 {IsDet @head}
      end
      meth getAll($) N in
	 nil = tail := N
	 head := N
      end
   end
   
   fun{NewFaultStream Init}
      C={NewCell _}
      P={NewPort @C} in
      {Send P Init}
      fs(get:fun{$}@C end
	 tell:proc{$ V}{Send P V}(O N in O = C:=N N=O.2)end)
   end
end