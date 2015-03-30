#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class WebUISampleApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void WebUISampleApp::setup()
{
}

void WebUISampleApp::mouseDown( MouseEvent event )
{
}

void WebUISampleApp::update()
{
}

void WebUISampleApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( WebUISampleApp, RendererGl )
