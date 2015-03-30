#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "WebSocketServer.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class WebUISampleApp : public App {
public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
    WebSocketServer				mServer;
    void						onConnect();
    void						onDisconnect();
    void						onError( std::string err );
    void						onInterrupt();
    void						onPing( std::string msg );
    void						onRead( std::string msg );
};

void WebUISampleApp::setup()
{
    mServer.addConnectCallback( &WebUISampleApp::onConnect, this );
    mServer.addDisconnectCallback( &WebUISampleApp::onDisconnect, this );
    mServer.addErrorCallback( &WebUISampleApp::onError, this );
    mServer.addInterruptCallback( &WebUISampleApp::onInterrupt, this );
    mServer.addPingCallback( &WebUISampleApp::onPing, this );
    mServer.addReadCallback( &WebUISampleApp::onRead, this );

    mServer.listen( 9002 );
}

void WebUISampleApp::mouseDown( MouseEvent event )
{
    mServer.write( "foo" );
}

void WebUISampleApp::update()
{
    mServer.poll();

}

void WebUISampleApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

void WebUISampleApp::onConnect()
{
    console() << "connect" << endl;
}

void WebUISampleApp::onDisconnect()
{
    console() << "disconnect" << endl;
}

void WebUISampleApp::onInterrupt()
{
    console() << "interrupt" << endl;
}

void WebUISampleApp::onError( string err )
{
    console() << "error: " << err << endl;
}

void WebUISampleApp::onPing( string msg )
{
    console() << "ping: " << msg << endl;
}

void WebUISampleApp::onRead( string msg )
{
    console() << "read: " << msg << endl;
}




CINDER_APP( WebUISampleApp, RendererGl )
