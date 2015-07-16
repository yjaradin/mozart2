functor
import
   BSerializer at 'x-oz://boot/Serializer'
   Site
export
   Break
   Kill
   GetFaultStream
define
   fun{GetDistrObject X}
      if {Value.type X} == object andthen {Site.is X} then
	 X
      else
	 GN T in
	 {BSerializer.getGNodeAndType X ?GN ?T}
	 {{Site.this getProtocolManager($)} makeProtocolObject(GN T $)}
      end
   end
   proc{Break X}
      {{GetDistrObject X} break()}
   end
   proc{Kill X}
      {{GetDistrObject X} kill()}
   end
   fun{GetFaultStream X}
      {{GetDistrObject X} getFaultStream($)}
   end
end