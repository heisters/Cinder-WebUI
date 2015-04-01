#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "WebUI.h"

class WebUISampleApp : public ci::app::App {
public:
    WebUISampleApp();

	void setup() override;
    void resize() override;
    void keyDown( ci::app::KeyEvent event ) override;
	void update() override;
	void draw() override;

private:
    ci::CameraPersp                 mCam;
    webui::WebUI                    mUI;
    webui::BoundParam< float >      mScale;
    webui::BoundParam< glm::vec3 >  mCenter;
};

using namespace ci;
using namespace ci::app;
using namespace std;

WebUISampleApp::WebUISampleApp() :
mScale( 0.5f )
{
}

void WebUISampleApp::setup()
{
    log::manager()->enableSystemLogging();
    log::manager()->setSystemLoggingLevel( log::LEVEL_INFO );
//    log::manager()->enableConsoleLogging();

    mUI.listen( 9002 );

    mUI.bind( "scale", &mScale );
    mUI.bind( "center", &mCenter );

    resize();
}

void WebUISampleApp::resize()
{
    mCam.lookAt( vec3( 0, 0, 10 ), vec3( 0 ) );
    mCam.setPerspective( 45.f, getWindowAspectRatio(), 1.f, 20.f );
}

void WebUISampleApp::keyDown( KeyEvent event )
{
    if ( event.getCode() == KeyEvent::KEY_UP )
    {
        mScale += 0.1;
    }

    else if ( event.getCode() == KeyEvent::KEY_DOWN )
    {
        mScale -= 0.1;
    }
}

void WebUISampleApp::update()
{
    mUI.update();

}

void WebUISampleApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
    gl::setMatrices( mCam );

    gl::drawColorCube( mCenter, vec3( 1.f ) * mScale() );
}


CINDER_APP( WebUISampleApp, RendererGl )
