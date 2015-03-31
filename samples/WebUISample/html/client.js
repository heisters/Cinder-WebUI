(function( exports ) {

/**
 * MicroEvent - to make any js object an event emitter (server or browser)
 * 
 * - pure javascript - server compatible, browser compatible
 * - dont rely on the browser doms
 * - super simple - you get it immediatly, no mistery, no magic involved
 *
 * - create a MicroEventDebug with goodies to debug
 *   - make it safer to use
*/

var MicroEvent	= function(){};
MicroEvent.prototype	= {
	bind	: function(event, fct){
		this._events = this._events || {};
		this._events[event] = this._events[event]	|| [];
		this._events[event].push(fct);
	},
	unbind	: function(event, fct){
		this._events = this._events || {};
		if( event in this._events === false  )	return;
		this._events[event].splice(this._events[event].indexOf(fct), 1);
	},
	trigger	: function(event /* , args... */){
		this._events = this._events || {};
		if( event in this._events === false  )	return;
		for(var i = 0; i < this._events[event].length; i++){
			this._events[event][i].apply(this, Array.prototype.slice.call(arguments, 1));
		}
	}
};

/**
 * mixin will delegate all MicroEvent.js function in the destination object
 *
 * - require('MicroEvent').mixin(Foobar) will make Foobar able to use MicroEvent
 *
 * @param {Object} the object which will support MicroEvent
*/
MicroEvent.mixin	= function(destObject){
	var props	= ['bind', 'unbind', 'trigger'];
	for(var i = 0; i < props.length; i ++){
		if( typeof destObject === 'function' ){
			destObject.prototype[props[i]]	= MicroEvent.prototype[props[i]];
		}else{
			destObject[props[i]] = MicroEvent.prototype[props[i]];
		}
	}
	return destObject;
}

///////////////////////////////////////////////////////////////////////////////

exports.Client = Client;
exports.ParamClient = ParamClient;

///////////////////////////////////////////////////////////////////////////////
// Event
UIEvent.Type = { UNKNOWN: "unknown", GET: "get", SET: "set" };

function UIEvent() {
}

Object.defineProperties( UIEvent.prototype, {
  "type": { value: UIEvent.Type.UNKNOWN, writable: true },
  "data": { value: undefined, writable: true },
} );

///////////////////////////////////////////////////////////////////////////////
// Client
function Client() {
}

Object.defineProperties( Client.prototype, {
  "events": { get: function() {
    if ( !this._events ) {
      this._events = {};
      MicroEvent.mixin( this._events );
    }

    return this._events;
  } },

  "connect": { value: function( url ) {
    this.url = url;
    console.log( "Connecting to " + this.url );

    if ("WebSocket" in window) {
      this.ws = new WebSocket( this.url );
    } else if ("MozWebSocket" in window) {
      this.ws = new MozWebSocket( this.url );
    } else {
      console.error( "This Browser does not support WebSockets" );
      return;
    }

    this.ws.onopen = this.onWSOpen.bind( this );
    this.ws.onerror = this.onWSError.bind( this );
    this.ws.onclose = this.onWSClose.bind( this );
    this.ws.onmessage = this.onWSMessage.bind( this );
  } },

  "disconnect": { value: function() {
    this.ws.close();
  } },

  "write": { value: function( msg ) {
    if ( !this.isReady ) {
      console.warn( "Trying to write to WebSocket that is not ready to write." );
      return;
    }

    this.ws.send( msg );
  } },

  "set": { value: function( name, value ) {
    var data = {};
    data[ name ] = value;
    this.write( JSON.stringify( {set: data} ) );
  } },

  "get": { value: function( name ) {
    this.write( JSON.stringify( {get: name} ) );
  } },

  "isReady": { get: function() {
    return this.ws && this.ws.readyState == 1;
  } },

  "onWSOpen": { value: function( event ) {
  } },

  "onWSError": { value: function( event ) {
    console.error( "ERROR:", event );
  } },

  "onWSClose": { value: function( event ) {
  } },

  "onWSMessage": { value: function( event ) {
    var parsed = {};

    try {
      parsed = JSON.parse( event.data );
    } catch( boom ) {
      console.warn( "Could not parse message:", event.data );
      return;
    }

    for ( var command in parsed ) {
      var event = new UIEvent();
      var data = parsed[ command ];

      if ( command === UIEvent.Type.GET || command === UIEvent.Type.SET ) {
        event.type = command;
        event.data = data;

        this.events.trigger( event.type, event );

      } else {
        console.warn( "Unrecognized message:", parsed );
      }
    }
  } }
} );

///////////////////////////////////////////////////////////////////////////////
// ParamClient
ParamClient.prototype = new Client();
ParamClient.prototype.constructor = ParamClient;
function ParamClient( el ) {
  Client.call( this );
  this.el = el;
  this.el.oninput = this.onElInput.bind( this );
  this.inputs = {};

  this.events.bind( UIEvent.Type.GET, this.onGet.bind( this ) );
  this.events.bind( UIEvent.Type.SET, this.onSet.bind( this ) );
}

Object.defineProperties( ParamClient.prototype, {
  "nameForElement": { value: function( el ) {
    return el.getAttribute( "name" ) || el.getAttribute( "id" );
  } },

  "bind": { value: function( input ) {
    var name = this.nameForElement( input );
    if ( !name ) return;

    this.inputs[ name ] = input;
  } },

  "sync": { value: function() {
    for( var n in this.inputs ) {
      this.get( n );
    }
  } },

  "onElInput": { value: function( event ) {
    event.stopPropagation();

    var name = this.nameForElement( event.target );
    if ( this.inputs[ name ] ) {
      this.set( name, event.target.value );
    }
  } },

  "onWSOpen": { value: function( event ) {
    this.sync();
  } },

  "onGet": { value: function( event ) {
    var name = event.data;
    if ( this.inputs[ name ] ) {
      this.set( name, this.inputs[ name ].value );
    }
  } },

  "onSet": { value: function( event ) {
    for ( var name in event.data ) {
      if ( this.inputs[ name ] ) {
        this.inputs[ name ].value = event.data[ name ];
      }
    }
  } }
} );


})( this.WebUIClient = {} );
