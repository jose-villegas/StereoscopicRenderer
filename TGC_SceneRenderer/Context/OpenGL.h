#pragma once
#include <windows.h>
#include "../Utils/FrameRate.h"
#include "../Core/Renderer.h"
#include "../Collections/Textures.h"
#include "LibInfo.h"
#include "Multisample/arb_multisample.h"
using namespace System::Windows::Forms;

namespace OpenGLForm {
    public ref class COpenGL : public System::Windows::Forms::NativeWindow {
        public:
            COpenGL(System::Windows::Forms::Panel ^parentForm, int iPositionX, int iPositionY, GLsizei iWidth, GLsizei iHeight);
            System::Void restartStopwatch(System::Void);
            System::Void calculateFramerate(System::Boolean blSet) { this->_calcFramerate = blSet; }
            System::Void swapOpenGLBuffers(System::Void) { SwapBuffers(_mHDC); }
            System::Double getDeltaTime() { return _deltaTime; }
            System::Double getTotalTime() { return _totalTime; }
            System::Double getFrameRate() { return _framerate; }
            virtual System::Void render(System::Void);
            System::String ^OGL_INFO_STRING;

        private:
            HDC _mHDC;
            HGLRC _mHGLRC;
            PIXELFORMATDESCRIPTOR *pfd;
            System::Diagnostics::Stopwatch _stopwatch;
            System::Double _deltaTime;
            System::Double _totalTime;
            System::Double _framerate;
            Utils::FrameRate _fmCalc;
            ECollections::Textures *_texCollection;
            System::Boolean _calcFramerate;
            Core::Renderer *_oglRender;
            HDC createHandle(System::Windows::Forms::Panel ^parentForm, int iPositionX, int iPositionY, GLsizei iWidth, GLsizei iHeight);

        protected:
            ~COpenGL(System::Void);
            GLint oglSetPixelFormat(HDC hdc);
    };
}