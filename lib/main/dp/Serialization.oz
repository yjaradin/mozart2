functor
import
   BSerializer at 'x-oz://boot/Serializer'
   System
export
   BaseSerializer
   BaseDeserializer
   Serializer
   Deserializer
define
   local
      Fin={Dictionary.new}
      WD={WeakDictionary.new
	  thread
	     for K#_ in $ do
		try
		   P=Fin.K in
		   {Dictionary.remove Fin K}
		   {P}
		catch E then
		   {System.show 'Serialization.oz error'#E} %shouldn't happen
		end
	     end
	  end}
      C={NewCell 0}
   in
      proc {Cleanup V P} %TODO: have (de)serializer deal with their own GCing
	 K=(O N in O=C:=N N=O+1 O) in %{NewName} in
	 Fin.K:=P
	 {WeakDictionary.put WD K V}
      end
   end

   class BaseSerializer
      attr ser key
      meth init(Value)
	 Resources
	 Ser = {BSerializer.newSerializer 'Mozart2_DSS'} in
	 key := {NewName}
	 {Cleanup @key proc{$} {BSerializer.releaseSerializer Ser} end}
	 ser := Ser
	 {BSerializer.setRoot @ser {BSerializer.add @ser [Value] nil [$]#nil#Resources}} 
	 {self handleResources(Resources)}
      end
      meth initWithImmediates(Immediates F)
	 Resources1 Resources2 Serializeds
	 Ser = {BSerializer.newSerializer 'Mozart2_DSS'} in
	 key := {NewName}
	 {Cleanup @key proc{$} {BSerializer.releaseSerializer Ser} end}
	 ser := Ser
	 {BSerializer.add @ser nil Immediates nil#Serializeds#Resources1}
	 {BSerializer.setRoot @ser {BSerializer.add @ser [{F Serializeds}] nil [$]#nil#Resources2}} 
	 {self handleResources({Append Resources1 Resources2})}
      end
      meth close(?Serialized)
	 {BSerializer.getSerialized @ser Serialized}
	 key:=_
	 ser:=_
      end
      meth handleResources(Resources)
	 NewResources in
	 for
	    R in Resources
	    Id in {BSerializer.add
		   @ser
		   {Map Resources fun{$ R}{self handleResource(R $)}end}
		   nil
		   $#nil#NewResources}
	 do
	    {BSerializer.putImmediate @ser R.1 Id}
	 end
	 if NewResources \= nil then
	    {self handleResources(NewResources)}
	 end
      end
      meth handleResource(Resource ?ProtocolData)
	 raise badResource(Resource) end
      end
   end

   class BaseDeserializer
      attr deser key
      meth init(Value)
	 Resources
	 Deser = {BSerializer.newDeserializer Value Resources} in
	 key := {NewName}
	 {Cleanup @key proc{$} {BSerializer.releaseDeserializer Deser} end}
	 deser := Deser
	 {self handleResources(Resources)}
      end
      meth close(?Deserialized)
	 {BSerializer.getRoot @deser Deserialized}
	 key := _
	 deser := _
      end
      meth handleResources(Resources)
	 for R in Resources do
	    {self handleResource(R)}
	 end
      end
      meth handleResource(Resource)
	 raise badResource(Resource) end
      end
      meth setImmediate(GNode Value)
	 {BSerializer.setImmediate @deser GNode Value}
      end
   end

   class Serializer
      from BaseSerializer
      attr
	 localSite
	 remoteSite
	 protocolManager
      meth init(LocalSite RemoteSite Value)
	 localSite := LocalSite
	 remoteSite := RemoteSite
	 protocolManager := {LocalSite getProtocolManager($)}
	 BaseSerializer,init(Value)
      end
      meth handleResource(R ?ProtocolData)
	 GN = R.2
      in
	 {@protocolManager makeGlobal(@remoteSite GN {Label R} ProtocolData)}
      end
   end

   class Deserializer
      from BaseDeserializer
      attr
	 localSite
	 remoteSite
	 protocolManager
      meth init(LocalSite RemoteSite Value)
	 localSite := LocalSite
	 remoteSite := RemoteSite
	 protocolManager := {LocalSite getProtocolManager($)}
	 BaseDeserializer,init(Value)
      end
      meth handleResource(R)
	 case R
	 of GN#Type#ProtocolData then
	    {@protocolManager makeProxy(@remoteSite GN Type ProtocolData)}
	 end
      end
   end
end
