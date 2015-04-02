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

///////////////////////////////////////////////////////////////////////////////
// UIEvent
UIEvent.Type = { UNKNOWN: "unknown", GET: "get", SET: "set" };

function UIEvent() {
}

Object.defineProperties( UIEvent.prototype, {
  "type": { value: UIEvent.Type.UNKNOWN, writable: true },
  "data": { value: undefined, writable: true },
} );

///////////////////////////////////////////////////////////////////////////////
// Client
function Client( el ) {
  this.el = el;
  this.el.oninput = this.onElInput.bind( this );
  this.inputs = {};

  this.events.bind( UIEvent.Type.GET, this.onGet.bind( this ) );
  this.events.bind( UIEvent.Type.SET, this.onSet.bind( this ) );
}

Object.defineProperties( Client.prototype, {
  "events": { get: function() {
    if ( !this._events ) {
      this._events = {};
      MicroEvent.mixin( this._events );
    }

    return this._events;
  } },

  "connect": { value: function( url, options ) {
    Object.defineProperty( this, "url", { value: url } );
    Object.defineProperty( this, "options", { value: options } );

    console.log( "Connecting to " + this.url );

    this.reconnect();
  } },

  "reconnect": { value: function() {
    if ( this.__persistConnectionTimeout ) clearTimeout( this.__persistConnectionTimeout );
    if ( this.isReady ) this.disconnect();
    this.ws = undefined;

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
    this.ws = undefined;
  } },

  "persistConnection": { value: function() {
    if ( !this.options.persistent ) return;
    if ( this.__persistConnectionTimeout ) clearTimeout( this.__persistConnectionTimeout );
    this.ws = undefined;
    this.__persistConnectionTimeout = setTimeout( function() { this.reconnect(); }.bind( this ), 250 );
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
    console.log( "Connected." );
    this.events.trigger( "connected", event );
  } },

  "onWSError": { value: function( event ) {
    this.persistConnection();
  } },

  "onWSClose": { value: function( event ) {
    this.events.trigger( "closed", event );
    this.persistConnection();
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
  } },

  "getNameForElement": { value: function( el ) {
    return el.getAttribute( "name" ) || el.getAttribute( "id" );
  } },

  "getValueForElement": { value: function( el ) {
    var value = undefined;

    if ( el.tagName === "INPUT" ) value = el.value;

    else if ( el.tagName === "FIELDSET" ) {
      var inputs = el.querySelectorAll( "input" );
      value = [];
      for ( var i = 0; i < inputs.length; ++i ) {
        value[ i ] = inputs[ i ].value;
      }
    }

    return value;
  } },

  "setValueForElement": { value: function( el, value ) {
    if ( el.tagName === "INPUT" ) {
      el.value = value;
      var event = new Event( 'change', { 'view': window, 'bubbles': true, 'cancelable': true } );
      el.dispatchEvent( event );
      var event = new Event( 'input', { 'view': window, 'bubbles': true, 'cancelable': true } );
      el.dispatchEvent( event );

    } else if ( el.tagName === "FIELDSET" ) {
      var inputs = el.querySelectorAll( "input" );
      for ( var i = 0; i < value.length; ++i ) {
        this.setValueForElement( inputs[ i ], value[ i ] );
      }
    }
  } },

  "hasInput": { value: function( name ) {
    return name in this.inputs;
  } },

  "getInput": { value: function( name ) {
    var input = this.inputs[ name ];
    if ( !input ) {
      console.warn( "unrecognized input:", name );
    }

    return input;
  } },

  "getParentFieldset": { value: function( el ) {
    var parent = el;
    while ( parent && parent.tagName !== "FIELDSET" ) parent = parent.parentElement;
    return parent;
  } },

  "bind": { value: function( input ) {
    var name = this.getNameForElement( input );
    if ( !name ) return;

    this.inputs[ name ] = input;
  } },

  "getAll": { value: function() {
    for( var n in this.inputs ) {
      this.get( n );
    }
  } },

  "onElInput": { value: function( event ) {
    var name = this.getNameForElement( event.target );
    if ( this.hasInput( name ) ) {
      this.set( name, this.getValueForElement( this.getInput( name ) ) );


    } else {
      var fs = this.getParentFieldset( event.target )
      if ( fs ) {
        name = this.getNameForElement( fs );
        this.set( name, this.getValueForElement( fs ) );

      } else {
        console.warn( "unrecognized input:", name );
      }
    }
  } },

  "onGet": { value: function( event ) {
    var name = event.data;
    var input = this.getInput( name );
    if ( input ) {
      this.set( name, this.getValueForElement( input ) );
    }
  } },

  "onSet": { value: function( event ) {
    for ( var name in event.data ) {
      var input = this.getInput( name );
      if ( input ) {
        this.setValueForElement( input, event.data[ name ] );
      }
    }
  } }
} );


})( this.WebUIClient = {} );
