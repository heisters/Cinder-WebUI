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
    webui::BoundParam< float >      mRadius;
    webui::BoundParam< glm::vec2 >  mCenter;
    webui::BoundParam< glm::vec3 >  mRotation;
    webui::BoundParam< int >        mNumCubes, mSubdivisions;
};

using namespace ci;
using namespace ci::app;
using namespace std;

WebUISampleApp::WebUISampleApp() :
mRadius( 0.5f ),
mCenter( vec2( 0.f ) ),
mRotation( vec3( 0.1f ) ),
mNumCubes( 1 ),
mSubdivisions( 12 )
{
}

void WebUISampleApp::setup()
{
    log::manager()->enableConsoleLogging();

    mUI.listen( 9002 );

    mUI.bind( "radius", &mRadius );
    mUI.bind( "center", &mCenter );
    mUI.bind( "rotation", &mRotation );
    mUI.bind( "num-cubes", &mNumCubes );
    mUI.bind( "subdivisions", &mSubdivisions );

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
        mRadius += 0.1;
    }

    else if ( event.getCode() == KeyEvent::KEY_DOWN )
    {
        mRadius -= 0.1;
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

    for ( int i = 0; i < mNumCubes; ++i )
    {
        gl::ScopedModelMatrix scp_mtx;
        vec2 center = mCenter() + vec2( float(i) * mRadius * 2.f + ( mNumCubes - 1 ) * -mRadius, 0.f );

        gl::translate( center );
        gl::rotate( 1.f, mRotation() * float( M_PI ) * 2.f );
        gl::drawSphere( vec3( 0.f ), mRadius, mSubdivisions );
    }
}


CINDER_APP( WebUISampleApp, RendererGl )
