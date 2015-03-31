(function( exports ) {

exports.Client = Client;
exports.ParamClient = ParamClient;

///////////////////////////////////////////////////////////////////////////////
// Client
function Client() {
}

Object.defineProperties( Client.prototype, {
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
    this.write( [ "set", name, value ].join( " " ) );
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

  "onElInput": { value: function( event ) {
    event.stopPropagation();

    var name = this.nameForElement( event.target );
    if ( this.inputs[ name ] ) {
      this.set( name, event.target.value );
    }
  } }
} );


})( this.WebUIClient = {} );
