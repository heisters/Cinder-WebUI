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
    ci::CameraPersp                     mCam;
    webui::WebUI                        mUI;
	webui::BoundParam< float >          mRadius = 0.5f;
	webui::BoundParam< float >          mFPS;
    webui::BoundParam< ci::vec2 >       mCenter;
    webui::BoundParam< ci::vec3 >       mRotation;
	webui::BoundParam< int >            mNumCubes = 1;
	webui::BoundParam< int >			mSubdivisions = 12;
    webui::BoundParam< std::string >    mText;
	webui::BoundParam< bool >			mYesOrNo = false;
	webui::BoundParam< ci::Colorf >		mColor;
	webui::BoundParam< std::vector< std::string > > mLetters;
    ci::Font                            mFont;
};

using namespace ci;
using namespace ci::app;
using namespace std;

WebUISampleApp::WebUISampleApp() :
mCenter( vec2( 0.f ) ),
mRotation( vec3( 0.1f ) ),
mText( "text" ),
mFont( loadAsset( "Roboto-Black.ttf" ), 16 ),
mColor( Colorf( 1.f, 0.f, 0.f ) ),
mLetters( vector< string >{"🐶","🐱","🐼"} )
{
}

void WebUISampleApp::setup()
{
    gl::enableAlphaBlending();

    mUI.listen( 9002 );

	mUI.bind( "fps", &mFPS );
    mUI.bind( "radius", &mRadius );
    mUI.bind( "center", &mCenter );
    mUI.bind( "rotation", &mRotation );
    mUI.bind( "num-cubes", &mNumCubes );
    mUI.bind( "subdivisions", &mSubdivisions );
    mUI.bind( "text", &mText );
	mUI.bind( "color", &mColor );
	mUI.bind( "yes-no", &mYesOrNo );
	mUI.bind( "letter", &mLetters );

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
	mFPS = getAverageFps();
    mUI.update();
}

void WebUISampleApp::draw()
{
	gl::clear();

	{
		gl::ScopedMatrices scp_mtx1;
		gl::ScopedColor scp_color( mColor() );
		gl::setMatrices( mCam );

		gl::rotate( 1.f, mRotation() * float( M_PI ) * 2.f );

		for ( int i = 0; i < mNumCubes; ++i )
		{
			gl::ScopedModelMatrix scp_mtx2;
			vec2 center = mCenter() + vec2( float(i) * mRadius * 2.f + ( mNumCubes - 1 ) * -mRadius, 0.f );

			gl::translate( center );
			gl::drawSphere( vec3( 0.f ), mRadius, mSubdivisions );
		}
	}

	{
		gl::ScopedMatrices scp_mtx1;
		gl::setMatricesWindow( getWindowSize() );
		gl::drawString( mText, vec2( 5, getWindowHeight() - (mFont.getSize()+5) ), ColorAf::white(), mFont );

		gl::drawString( mYesOrNo ? "🙂" : "🙃", vec2( 5, 5 ), ColorAf::white(), mFont );

		auto selected = mLetters.selected();
		stringstream ss;
		ss << "Selected letter: " << selected;
		gl::drawString( ss.str(), vec2( 5, mFont.getSize() + 5 * 2 ), ColorAf::white(), mFont );
	}
}


CINDER_APP( WebUISampleApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
