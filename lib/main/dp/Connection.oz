%%%
%%% Authors:
%%%   Yves Jaradin (yves.jaradin@uclouvain.be)
%%%   Raphael Collet (raphael.collet@uclouvain.be)
%%%
%%% Copyright:
%%%   Université catholique de Louvain, 2008-2015
%%%
%%% This file is part of Mozart, an implementation
%%% of Oz 3
%%%    http://www.mozart-oz.org
%%%
%%% See the file "LICENSE" or
%%%    http://www.mozart-oz.org/LICENSE.html
%%% for information on usage and redistribution
%%% of this file, and for a DISCLAIMER OF ALL
%%% WARRANTIES.
%%%

functor
import
   URL
   Site
   Channel

export
   offer:          OfferOnce
   offerUnlimited: OfferMany
   OfferOnce
   OfferMany
   Retract
   Take
   Gate

define
   %% initialize the ticket service
   Tickets={NewDictionary}
   Requests={NewDictionary}
   {Site.this
    registerDispatchingTarget(ticket
			      proc {$ From M}
				 case M
				 of ticketAsk(to:ticket Id ReqId) then
				    T = {Dictionary.condGet Tickets Id notFound} in
				    {From sendMessage(ticketReceive(to:ticket T ReqId))}
				 [] ticketAckOnce(to:ticket Id) then
				       {Dictionary.remove Tickets Id}
				 [] ticketReceive(to:ticket Val ReqId) then
				    {Dictionary.condGet Requests ReqId _}=Val
				    {Dictionary.remove Requests ReqId}
				 end
			      end _)}

   %% obtain the value corresponding to a given ticket
   fun {Take Ticket}
      {Channel.init}
      SiteURI#TicketId={ParseTicket Ticket}
      RemoteSite = {Site.this getRemoteByUrl({VirtualString.toAtom SiteURI} $)}
      ReqId={NewName}
      V
   in
      Requests.ReqId:=V
      {RemoteSite sendMessage(ticketAsk(to:ticket TicketId ReqId))}
      case V
      of found(X Kind) then
	 if Kind == once then
	    {RemoteSite sendMessage(ticketAckOnce(to:ticket TicketId))}
	 end
	 X
      else
	 {Exception.raiseError dp(ticket bad Ticket)}
	 unit
      end
   end

   %% retract a ticket
   proc{Retract Ticket}
      _#TicketId={ParseTicket Ticket}
   in
      {Dictionary.remove Tickets TicketId}
   end

   %% offer a value
   fun {OfferOnce X}
      {DoOffer X once}
   end
   fun {OfferMany X}
      {DoOffer X many}
   end

   fun {DoOffer X Kind}
      {Channel.init}
      T={NewTicketId}
   in
      Tickets.T:=found(X Kind)
      {MakeTicket T}
   end
   local C={NewCell 0} in
      fun {NewTicketId}
         I J in I=C:=J  J=I+1  I
      end
   end

   %% utils: create and parse tickets
   fun {MakeTicket Key}
      URI = case {Map {Sort {Site.this getReachability($)}.addresses fun{$ A1 A2}A1.scope > A2.scope end} fun{$ A}A.url end}
	    of U|_ then U
	    [] nil then "x-dss-unreachable://"
	    end
   in
      try
	 {URL.toVirtualStringExtended {Record.adjoinAt {URL.make URI} fragment Key} opt(full:true)}
      catch _ then
         {Exception.raiseError dp(ticket make URI)}
	 unit
      end
   end
   fun {ParseTicket VS}
      T={URL.make VS}
   in
      {URL.toVirtualString T}#{StringToInt T.fragment}
   end

   %% Gates: an alternative class for managing tickets
   class Gate
      feat Ticket
      meth init(X ?AT<=_)
         AT = (self.Ticket = {OfferMany X})
      end
      meth getTicket($)
         self.Ticket
      end
      meth close
         {Retract self.Ticket}
      end
   end
end
