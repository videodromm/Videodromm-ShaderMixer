#include "VideodrommShaderMixerApp.h"
void VideodrommShaderMixerApp::prepare(Settings *settings)
{
	settings->setWindowSize(40, 10);
	settings->setBorderless();
}
void VideodrommShaderMixerApp::setup()
{
	CI_LOG_V("Controller");
	// Settings
	mVDSettings = VDSettings::create();
	// Session
	mVDSession = VDSession::create(mVDSettings);

	setFrameRate(mVDSession->getTargetFps());
	mMovieDelay = mFadeInDelay = true;
	mVDSession->getWindowsResolution();

	getWindow()->getSignalResize().connect(std::bind(&VideodrommShaderMixerApp::resizeWindow, this));
	getWindow()->getSignalDraw().connect(std::bind(&VideodrommShaderMixerApp::drawRenderWindow, this));

	// mouse cursor and ui
	setUIVisibility(mVDSettings->mCursorVisible);
	// UI
	mVDUI = VDUI::create(mVDSettings, mVDSession);

	setFrameRate(mVDSession->getTargetFps());
	CI_LOG_V("setup");
}
/*void VideodrommShaderMixerApp::positionRenderWindow() {
	mVDSettings->mRenderPosXY = ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY);//20141214 was 0
	setWindowPos(mVDSettings->mRenderX, mVDSettings->mRenderY);
	setWindowSize(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
}*/
void VideodrommShaderMixerApp::createControlWindow()
{
	mVDUI->resize();

	deleteControlWindows();
	mVDSession->getWindowsResolution();
	CI_LOG_V("createRenderWindow, resolution:" + toString(mVDSettings->mMainWindowWidth) + "x" + toString(mVDSettings->mMainWindowHeight));

	string windowName = "render";
	// OK mControlWindow = createWindow(Window::Format().size(mVDSettings->mMainWindowWidth, mVDSettings->mMainWindowHeight));
	mControlWindow = createWindow(Window::Format().size(mVDSettings->mMainWindowWidth - 300, mVDSettings->mMainWindowHeight - 30));
	mControlWindow->setPos(mVDSettings->mMainWindowX, mVDSettings->mMainWindowY + 10);
	//mControlWindow->setBorderless();
	mControlWindow->getSignalDraw().connect(std::bind(&VideodrommShaderMixerApp::drawControlWindow, this));
	mControlWindow->getSignalResize().connect(std::bind(&VideodrommShaderMixerApp::resizeWindow, this));

	// create instance of the window and store in vector
	allWindows.push_back(mControlWindow);
	mVDSettings->mRenderPosXY = ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY);
}

void VideodrommShaderMixerApp::deleteControlWindows()
{
#if defined( CINDER_MSW )
	for (auto wRef : allWindows) DestroyWindow((HWND)mControlWindow->getNative());
#endif
	allWindows.clear();
}
void VideodrommShaderMixerApp::setUIVisibility(bool visible)
{
	if (visible)
	{
		showCursor();
	}
	else
	{
		hideCursor();
	}
}
void VideodrommShaderMixerApp::fileDrop(FileDropEvent event)
{
	mVDSession->fileDrop(event);
}

void VideodrommShaderMixerApp::cleanup()
{
	CI_LOG_V("shutdown");
	mVDSettings->save();
	mVDSession->save();
	ui::Shutdown();
	quit();
}
void VideodrommShaderMixerApp::mouseMove(MouseEvent event)
{
	if (!mVDSession->handleMouseMove(event)) {
		// let your application perform its mouseMove handling here
	}
}
void VideodrommShaderMixerApp::mouseDown(MouseEvent event)
{
	if (!mVDSession->handleMouseDown(event)) {
		// let your application perform its mouseDown handling here
	}
}
void VideodrommShaderMixerApp::mouseDrag(MouseEvent event)
{
	if (!mVDSession->handleMouseDrag(event)) {
		// let your application perform its mouseDrag handling here
	}	
}
void VideodrommShaderMixerApp::mouseUp(MouseEvent event)
{
	if (!mVDSession->handleMouseUp(event)) {
		// let your application perform its mouseUp handling here
	}
}

void VideodrommShaderMixerApp::keyDown(KeyEvent event)
{
	if (!mVDSession->handleKeyDown(event)) {
		switch (event.getCode()) {
		case KeyEvent::KEY_KP_PLUS:
			createControlWindow();
			break;
		case KeyEvent::KEY_KP_MINUS:
			deleteControlWindows();
			break;
		case KeyEvent::KEY_ESCAPE:
			// quit the application
			quit();
			break;
		case KeyEvent::KEY_h:
			// mouse cursor and ui visibility
			mVDSettings->mCursorVisible = !mVDSettings->mCursorVisible;
			setUIVisibility(mVDSettings->mCursorVisible);
			break;
		}
	}
}
void VideodrommShaderMixerApp::keyUp(KeyEvent event)
{
	if (!mVDSession->handleKeyUp(event)) {
	}
}
void VideodrommShaderMixerApp::resizeWindow()
{
	mVDUI->resize();
	if (mVDSettings->mStandalone) {
		// set ui window and io events callbacks
		ui::disconnectWindow(getWindow());
	}
	else {
		ui::disconnectWindow(mControlWindow);
	}
}
void VideodrommShaderMixerApp::update()
{
	switch (mVDSession->getCmd()) {
	case 0:
		createControlWindow();
		break;
	case 1:
		deleteControlWindows();
		break;
	}
	mVDSession->setFloatUniformValueByIndex(mVDSettings->IFPS, getAverageFps());
	mVDSession->update();
}

void VideodrommShaderMixerApp::drawRenderWindow()
{
	getWindow()->setTitle("(" + mVDSettings->sFps + " fps) " + toString(mVDSettings->iBeat) + " Videodromm");
	//renderSceneToFbo();
	if (mFadeInDelay) {
		mVDSettings->iAlpha = 0.0f;
		if (getElapsedFrames() > mVDSession->getFadeInDelay()) {
			mFadeInDelay = false;
			setWindowSize(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight);
			setWindowPos(ivec2(mVDSettings->mRenderX, mVDSettings->mRenderY));
			// warps resize at the end
			mVDSession->resize();
			timeline().apply(&mVDSettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}
	/*if (mMovieDelay) {
	if (getElapsedFrames() > mVDSession->getMoviePlaybackDelay()) {
	mMovieDelay = false;
	fs::path movieFile = getAssetPath("") / mVDSettings->mAssetsPath / mVDSession->getMovieFileName();
	mVDTextures->loadMovie(0, movieFile.string());
	}
	}*/
	gl::clear(Color::black());
	gl::setMatricesWindow(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, false);
	gl::draw(mVDSession->getRenderTexture(), getWindowBounds());
}

void VideodrommShaderMixerApp::drawControlWindow()
{
	gl::clear(Color::black());
	//gl::color(Color::white());
	gl::setMatricesWindow(mVDSettings->mRenderWidth, mVDSettings->mRenderHeight, false);
	gl::draw(mVDSession->getMixTexture(mVDSession->getCurrentEditIndex()), Rectf(10 + mVDSettings->uiLargeW, 170, 650 + mVDSettings->uiLargeW, 650));
	//gl::draw(mVDSession->getMixTexture(), Rectf(0, 170, 350 , 350));
	if (mVDSettings->mCursorVisible) {
		// imgui
		mVDUI->Run("UI", (int)getAverageFps());
		if (mVDUI->isReady()) {
		}

	}
}


CINDER_APP(VideodrommShaderMixerApp, RendererGl)
