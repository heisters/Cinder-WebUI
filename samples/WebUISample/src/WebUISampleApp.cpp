#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "WebUI.h"

class WebUISampleApp : public ci::app::App {
public:
    WebUISampleApp();

	void setup() override;
    void mouseDown( ci::app::MouseEvent event ) override;
	void update() override;
	void draw() override;

private:

    webui::ParamUI  mUI;
    float           mWidth;
};

using namespace ci;
using namespace ci::app;
using namespace std;

WebUISampleApp::WebUISampleApp() :
mWidth( 1.f )
{
}

void WebUISampleApp::setup()
{
    log::manager()->enableSystemLogging();
    log::manager()->setSystemLoggingLevel( log::LEVEL_INFO );
//    log::manager()->enableConsoleLogging();

    mUI.listen( 9002 );

    mUI.addParam( "width", &mWidth );
}

void WebUISampleApp::mouseDown( MouseEvent event )
{
}

void WebUISampleApp::update()
{
    mUI.update();

}

void WebUISampleApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );

    gl::ScopedColor scp_color( ColorAf( 1.f, 0.0, 0.0, 1.0 ) );

    Rectf r = getWindowBounds();
    r.scaleCentered( vec2( mWidth, 1.f ) );
    gl::drawSolidRect( r );
}





CINDER_APP( WebUISampleApp, RendererGl )
