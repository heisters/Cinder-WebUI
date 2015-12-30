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
		for(var i = 0; i < this._events[event].length; ++i){
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

// Simple JavaScript Templating
// John Resig - http://ejohn.org/ - MIT Licensed
var Template = (function(){
  var cache = {};

  return function tmpl(str, data){
    // Figure out if we're getting a template, or if we need to
    // load the template - and be sure to cache the result.
    var fn = !/\W/.test(str) ?
      cache[str] = cache[str] ||
        tmpl(document.getElementById(str).innerHTML) :

      // Generate a reusable function that will serve as a template
      // generator (and which will be cached).
      new Function("obj",
        "var p=[],print=function(){p.push.apply(p,arguments);};" +

        // Introduce the data as local variables using with(){}
        "with(obj){p.push('" +

        // Convert the template into pure JavaScript
        str
          .replace(/[\r\t\n]/g, " ")
          .split("<%").join("\t")
          .replace(/((^|%>)[^\t]*)'/g, "$1\r")
          .replace(/\t=(.*?)%>/g, "',$1,'")
          .split("\t").join("');")
          .split("%>").join("p.push('")
          .split("\r").join("\\'")
      + "');}return p.join('');");

    // Provide some basic currying to the user
    return data ? fn( data ) : fn;
  };
})();

///////////////////////////////////////////////////////////////////////////////
// http://www.html5rocks.com/en/tutorials/pagevisibility/intro/
var Visibility = ( function() {
  var getHiddenProp = function(){
    var prefixes = ['webkit','moz','ms','o'];

    // if 'hidden' is natively supported just return it
    if ('hidden' in document) return 'hidden';

    // otherwise loop over all the known prefixes until we find one
    for (var i = 0; i < prefixes.length; i++){
        if ((prefixes[i] + 'Hidden') in document) 
            return prefixes[i] + 'Hidden';
    }

    // otherwise it's not supported
    return null;
  };

  var isHidden = function() {
    var prop = getHiddenProp();
    if (!prop) return false;

    return document[prop];
  };

  var onVisibilityChange = function( cb ) {
    var visProp = getHiddenProp();
    if (visProp) {
      var evtname = visProp.replace(/[H|h]idden/,'') + 'visibilitychange';
      document.addEventListener(evtname, cb);
    }
  };

  var onVisible = function( cb ) {
    if ( !isHidden() ) {
      cb();
      return;
    }

    onVisibilityChange( cb );
  };

  return {
    onVisibilityChange: onVisibilityChange,
    onVisible: onVisible,
    isHidden: isHidden
  };
} )();

///////////////////////////////////////////////////////////////////////////////
// writeDOMAsync
var writeDOMAsync = (function(){
  var callbacks = {};

  return function( id, cb ) {
    if ( !id ) {
      console.warn( "Performing unmanaged async DOM write." );
      requestAnimationFrame( cb );
      return;
    }

    if ( callbacks[ id ] ) cancelAnimationFrame( callbacks[ id ] );
    callbacks[ id ] = requestAnimationFrame( function() {
      cb();
      delete callbacks[ id ];
    } );
  };
})();

///////////////////////////////////////////////////////////////////////////////
function camelCaseString( string ) {
  return string.replace( /^([A-Z])|[\s-_](\w)/g, function( match, p1, p2, offset ) {
      if ( p2 ) return p2.toUpperCase();
      return p1.toLowerCase();
  } );
}
///////////////////////////////////////////////////////////////////////////////

exports.Client = Client;

///////////////////////////////////////////////////////////////////////////////
// ServerEvent
ServerEvent.Type = { UNKNOWN: "unknown", GET: "get", SET: "set" };

function ServerEvent() {
}

Object.defineProperties( ServerEvent.prototype, {
  type: { value: ServerEvent.Type.UNKNOWN, writable: true },
  data: { value: undefined, writable: true },
} );

///////////////////////////////////////////////////////////////////////////////
// UserEvent
UserEvent.Type = { UNKNOWN: "unknown", INPUT: "input", SELECT: "select", SET: "set" };

function UserEvent() {
}

Object.defineProperties( UserEvent.prototype, {
  type: { value: UserEvent.Type.UNKNOWN, writable: true },
  preventDefault: { value: false, writable: true },
  client: { value: undefined, writable: true },
  target: { value: undefined, writable: true },
  binding: { value: undefined, writable: true },

  select: { value: function( id, value ) {
    this.client.select( id, value );
  } },

  set: { value: function( id, value ) {
    this.client.set( id, value );
  } },

  clone: { value: function() {
    var event             = new UserEvent();
    event.type            = this.type;
    event.preventDefault  = this.preventDefault;
    event.client          = this.client;
    event.target          = this.target;
    event.binding         = this.binding;
    return event;
  } }
} );

///////////////////////////////////////////////////////////////////////////////
// DataConverter
function DataConversionException( from, to ) {
  this.message = "Cannot convert from " + from + " to " + to;
}

function DataConverter( raw, type ) {
  this.raw = raw;
  // type must be one of the conversion functions below
  this.type = type;
}

Object.defineProperties( DataConverter.prototype, {
  is: { value: function() {
    return Array.prototype.slice.call( arguments ).indexOf( this.type ) !== -1;
  } },

  // Server types, from HTML input types

  float: { get: function() {
    if ( !this.is( "range", "number", "text" ) ) throw new DataConversionException( this.type, "float" );
    return parseFloat( this.raw );
  } },

  int: { get: function() {
    if ( !this.is( "range", "number", "text" ) ) throw new DataConversionException( this.type, "int" );
    return parseInt( this.raw, 10 );
  } },

  vec3: { get: function() {
    if ( !this.is( "color" ) ) throw new DataConversionException( this.type, "vec3" );

    var bigint = parseInt( this.raw.replace( /[^0-9A-F]/gi, '' ), 16 );
    return [
      ( (bigint >> 16) & 255 ) / 255,
      ( (bigint >> 8) & 255 ) / 255,
      ( bigint & 255 ) / 255
    ];
  } },


  // HTML input types, from Server types

  text: { get: function() {
    if ( !this.is( "float", "int" ) ) throw new DataConversionException( this.type, "text" );
    return this.raw;
  } },

  number: { get: function() {
    if ( !this.is( "float", "int" ) ) throw new DataConversionException( this.type, "number" );
    return this.raw;
  } },

  range: { get: function() {
    if ( !this.is( "float", "int" ) ) throw new DataConversionException( this.type, "range" );
    return this.raw;
  } },

  color: { get: function() {
    if ( !this.is( "vec3" ) ) throw new DataConversionException( this.type, "color" );

    var rgb = this.raw.map( function( f ) { return Math.floor( f * 255 ); } );
    return "#" + ((1 << 24) + (rgb[0] << 16) + (rgb[1] << 8) + rgb[2]).toString(16).slice(1);
  } }
} );

////////////////////////////////////////////////////////////////////////////////
// Binding
function Binding( id ) {
  Object.defineProperty( this, "id", { value: id } );
}

// Class methods ---------------------------------------------------------------
Object.defineProperties( Binding, {
  all: { get: function() {
    if ( !this._all ) this._all = {};
    return this._all;
  } },

  register: { value: function( binding ) {
    if ( binding.id in Binding.all ) {
      console.warn( "Binding " + binding.id + " already registered. Ids must be unique." );
      return;
    }
    Binding.all[ binding.id ] = binding;
  } },

  getAllValues: { value: function() {
    var values = {};
    for ( var id in this.all ) {
      values[ camelCaseString( id ) ] = this.all[ id ].value;
    }
    return values;
  } }
} );

// Instance methods ------------------------------------------------------------
Object.defineProperties( Binding.prototype, {
  events: { get: function() {
    if ( !this._events ) {
      this._events = {};
      MicroEvent.mixin( this._events );
    }

    return this._events;
  } },

  value: {
    get: function() {
      return this._value;
    },
    set: function( v ) {
      this._value = v;
    }
  },

  select: { value: function( listener ) {
    this.events.bind( UserEvent.Type.SELECT, listener );
  } },

  set: { value: function( listener ) {
    this.events.bind( UserEvent.Type.SET, listener );
  } },

  input: { value: function( listener ) {
    this.events.bind( UserEvent.Type.INPUT, listener );
  } }
} );

DOMBinding.prototype = Object.create( Binding.prototype );
DOMBinding.constructor = DOMBinding;
function DOMBinding( el ) {
  Object.defineProperty( this, "el", { value: el } );

  // Find and reference children
  var children = [];
  if ( this.el.tagName === "FIELDSET" ) {
    var inputs = this.el.querySelectorAll( "input" );
    for ( var i = 0; i < inputs.length; ++i ) {
      children[ i ] = new DOMBinding( inputs[ i ] );
    }
  }

  Object.defineProperty( this, "children", { value: children } );

  // Events
  this.events.bind( UserEvent.Type.INPUT, this.onUserInput.bind( this ) );

  // Call parent constructor
  Binding.call( this, DOMBinding.bindingIdForElement( this.el ) );
}

Object.defineProperties( DOMBinding, {
  // prefers general to specific
  bindingIdForElement: { value: function( el ) {
    return el.dataset.bindingId || el.getAttribute( "name" ) || el.getAttribute( "id" );
  } },

  uniqueBindingIdForElement: { value: (function() {
    var id = 0;
    return function( el ) {
      if ( el.dataset.uniqueBindingId ) return el.dataset.uniqueBindingId;
      el.dataset.uniqueBindingId = "__generated_unique_binding_id_" + (++id);
      return el.dataset.uniqueBindingId;
    }
  })() }
} );

Object.defineProperties( DOMBinding.prototype, {
  onUserInput: { value: function( event ) {
    // Translate event to select/set
    var specificEvent = event.clone();
    specificEvent.type = this.el.tagName === "SELECT" ? UserEvent.Type.SELECT : UserEvent.Type.SET;
    this.events.trigger( specificEvent.type, specificEvent );

    if ( specificEvent.preventDefault || event.preventDefault ) return;

    if ( this.el.tagName === "SELECT" ) {
      event.select( this.id, this.value );
    } else {
      event.set( this.id, this.value );
    }
  } },

  value: {
    get: function() {
      var value = undefined;

      if ( this.el.tagName === "INPUT" || this.el.tagName === "SELECT" ) {
        var prop = this.el.type === "checkbox" ? 'checked' : 'value';
        if ( this.el.dataset.serverType ) {
          value = (new DataConverter( this.el[prop], this.el.type ))[ this.el.dataset.serverType ];
        } else {
          value = this.el[prop];
        }

      } else if ( this.el.tagName === "SCRIPT" ) {
        value = this._value;

      } else if ( this.el.tagName === "FIELDSET" ) {
        value = this.children.map( function( c ) { return c.value; } );
      }

      return value;
    },

    set: function( value ) {
      writeDOMAsync( this.el.id, function() {
        if ( this.el.tagName === "INPUT" ) {
          var prop = this.el.type === "checkbox" ? 'checked' : 'value';
          if ( this.el.dataset.serverType ) {
            this.el[prop]= (new DataConverter( value, this.el.dataset.serverType ))[ this.el.type ];
          } else {
            this.el[prop] = value;
          }

          // Trigger a custom event so that the user can distinguish between user
          // input and this.
          var event = new Event( 'webui-set', { 'view': window, 'bubbles': true, 'cancelable': true } );
          this.el.dispatchEvent( event );


        } else if ( this.el.tagName === "SELECT" ) {
          // clear all content, this is faster than setting innerHTML
          while( this.el.lastChild ) this.el.removeChild( this.el.lastChild );

          if ( this.el.dataset.serverType ) console.warn( "Setting a serverType for a <select> is not yet supported." );

          value.forEach( function( v ) {
            var opt = document.createElement( "option" )
            opt.value = v;
            opt.innerHTML = v;
            this.el.appendChild( opt );
          }.bind( this ) );


        } else if ( this.el.tagName === "FIELDSET" ) {
          for ( var i = 0; i < value.length; ++i ) {
            this.children[ i ].value = value[ i ];
          }


        } else if ( this.el.tagName === "SCRIPT" ) {
          this._value = value;
          var result = Template( this.el.innerHTML, Binding.getAllValues() );
          document.getElementById( this.el.dataset.resultId ).innerHTML = result;
        }
      }.bind( this ) );
    }
  }
} );


SelectorBinding.prototype = Object.create( Binding.prototype );
SelectorBinding.constructor = SelectorBinding;
function SelectorBinding( id, selector ) {
  Object.defineProperty( this, "selector", { value: selector } );

  this.events.bind( UserEvent.Type.INPUT, this.onUserInput.bind( this ) );

  Binding.call( this, id );
}

Object.defineProperties( SelectorBinding.prototype, {
  matches: { value: function( el ) {
    return el.matches( this.selector );
  } },

  onUserInput: { value: function( event ) {
    var child = this.bindingForElement( event.target );
    var childEvent = event.clone();
    childEvent.binding = child;
    child.events.trigger( event.type, childEvent );
  } },

  bindingForElement: { value: function( el ) {
    if ( !this.children ) Object.defineProperty( this, "children", { value: {} } );

    var id = DOMBinding.uniqueBindingIdForElement( el );
    if ( this.children[ id ] ) return this.children[ id ];

    var binding = new DOMBinding( el );

    // Forward events
    [ UserEvent.Type.SET, UserEvent.Type.SELECT ].forEach( function( type ) {
      binding.events.bind( type, function( event ){
        this.events.trigger.call( this.events, type, event );
      }.bind( this ) );
    }.bind( this ) );

    this.children[ id ] = binding;
    return binding;
  } }
} );

///////////////////////////////////////////////////////////////////////////////
// Client
function Client( el ) {
  this.el = el;
  this.el.addEventListener( "change", this.onElChange.bind( this ) );
  this.el.addEventListener( "input", this.onElInput.bind( this ) );

  this.events.bind( ServerEvent.Type.GET, this.onServerGet.bind( this ) );
  this.events.bind( ServerEvent.Type.SET, this.onServerSet.bind( this ) );
}

Object.defineProperties( Client.prototype, {
  events: { get: function() {
    if ( !this._events ) {
      this._events = {};
      MicroEvent.mixin( this._events );
    }

    return this._events;
  } },

  bindings: { get: function() {
    if ( !this._bindings ) this._bindings = {};
    return this._bindings;
  } },

  connect: { value: function( url, options ) {
    Object.defineProperty( this, "url", { value: url } );
    Object.defineProperty( this, "options", { value: options } );

    console.log( "Connecting to " + this.url );

    this.reconnect();
  } },

  reconnect: { value: function() {
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

  disconnect: { value: function() {
    this.ws.close();
    this.ws = undefined;
  } },

  persistConnection: { value: function() {
    if ( !this.options.persistent ) return;
    Visibility.onVisible( function() {
      if ( this.__persistConnectionTimeout ) clearTimeout( this.__persistConnectionTimeout );
      this.ws = undefined;
      this.__persistConnectionTimeout = setTimeout( function() { this.reconnect(); }.bind( this ), 250 );
    }.bind( this ) );
  } },

  write: { value: function( msg ) {
    if ( !this.isReady ) {
      console.warn( "Trying to write to WebSocket that is not ready to write." );
      return;
    }

    this.ws.send( msg );
  } },

  set: { value: function( name, value ) {
    var data = {};
    data[ name ] = value;
    this.write( JSON.stringify( {set: data} ) );
  } },

  select: { value: function( name, value ) {
    var data = {};
    data[ name ] = value;
    this.write( JSON.stringify( {select: data} ) );
  } },

  get: { value: function( name ) {
    this.write( JSON.stringify( {get: name} ) );
  } },

  isReady: { get: function() {
    return this.ws && this.ws.readyState == 1;
  } },

  onWSOpen: { value: function( event ) {
    console.log( "Connected." );
    this.events.trigger( "connected", event );
  } },

  onWSError: { value: function( event ) {
    this.persistConnection();
  } },

  onWSClose: { value: function( event ) {
    this.events.trigger( "closed", event );
    this.persistConnection();
  } },

  onWSMessage: { value: function( event ) {
    var parsed = {};

    try {
      parsed = JSON.parse( event.data );
    } catch( boom ) {
      console.warn( "Could not parse message:", event.data );
      return;
    }

    for ( var command in parsed ) {
      var event = new ServerEvent();
      var data = parsed[ command ];

      if ( command === ServerEvent.Type.GET || command === ServerEvent.Type.SET ) {
        event.type = command;
        event.data = data;

        this.events.trigger( event.type, event );

      } else {
        console.warn( "Unrecognized message:", parsed );
      }
    }
  } },

  hasBinding: { value: function( id ) {
    return id in this.bindings;
  } },

  getBinding: { value: function( id ) {
    var binding = this.bindings[ id ];
    if ( !binding ) {
      console.warn( "unrecognized input:", id );
    }

    return binding;
  } },

  getParentFieldset: { value: function( el ) {
    var parent = el;
    while ( parent && parent.tagName !== "FIELDSET" ) parent = parent.parentElement;
    return parent;
  } },

  bind: { value: function( elOrString, selector ) {
    var binding = undefined;
    if ( typeof elOrString === "string" ) {
      if ( selector ) {
        binding = new SelectorBinding( elOrString, selector );
      } else {
        binding = new Binding( elOrString ); // simple data binding
      }
    } else {
      binding = new DOMBinding( elOrString );
    }
    Binding.register( binding );

    var id = binding.id;
    if ( !id ) return;
    this.bindings[ id ] = binding;
    return binding;
  } },

  getAll: { value: function() {
    for( var n in this.bindings ) this.get( n );
  } },

  handleElementInput: { value: function( el ) {
    var id = DOMBinding.bindingIdForElement( el );
    if ( !this.hasBinding( id ) ) return false;

    var binding = this.getBinding( id );
    if ( binding.matches && !binding.matches( el ) ) return false;

    var event = new UserEvent();
    event.type = UserEvent.Type.INPUT;
    event.target = el;
    event.binding = binding;
    event.client = this;

    binding.events.trigger( event.type, event );
    return true;
  } },

  onElInput: { value: function( event ) {
    var handled = false;
    handled = this.handleElementInput( event.target );

    if ( !handled ) {
      var fs = this.getParentFieldset( event.target )
      if ( fs  ) {
        handled = this.handleElementInput( fs );
      }
    }

    if ( !handled ) {
      console.warn( "unrecognized input:", event.target );
    }
  } },

  onElChange: { value: function( event ) {
    if ( event.target.tagName !== "INPUT" || event.target.type !== "checkbox" )
      return;

    this.onElInput( event );
  } },

  onServerGet: { value: function( event ) {
    var id = event.data;
    var binding = this.getBinding( id );
    if ( binding ) {
      this.set( id, binding.value );
    }
  } },

  onServerSet: { value: function( event ) {
    for ( var id in event.data ) {
      var binding = this.getBinding( id );
      if ( binding ) {
        binding.value = event.data[ id ];
        binding.events.trigger( event.type, event );
      }
    }
  } }
} );


})( this.WebUIClient = {} );
