/*
   Marius C. O. (circinusX1) all rights reserved
   FreeBSD License (c) 2005- 2020, comarius <marrius9876@gmail.com>
   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
*/
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include "screenxy.h"

#ifdef GL_DISPLAY
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

screenxy* Term;
extern bool __alive;



void block_signal(int signal_to_block /* i.e. SIGPIPE */ )
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, signal_to_block);
    sigset_t old_state;
    sigprocmask(SIG_BLOCK, &set, &old_state);
}

void notapl(int s)
{
    (void)s;
}

void ctrlc(int s)
{
    (void)s;
    __alive=false;
    ::sleep(1);
}

screenxy::screenxy(int argc, char* argv[])
{
    block_signal(SIGPIPE);
    signal(SIGINT, ctrlc);

    Term = this;
#ifdef GL_DISPLAY
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE);
    glutInitWindowSize(WIN_GL_W, 600);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("OpenGL - Creating a triangle");

    _max=0;

    //glutDisplayFunc(drawTriangle);
#else
    _pw = initscr();
    cbreak();
    noecho();
    clear();
    curs_set(0);
#endif
}

screenxy::~screenxy()
{
#ifdef GL_DISPLAY

#else
    endwin();
#endif
}

#ifdef GL_DISPLAY
void screenxy::add(double s, int diff)
{
    std::lock_guard<std::mutex> guard(_m);
    _delays.push_back(s);
    _diff.push_back(diff);
    _max = std::max(_max,float(s));
    if(_delays.size()>GRAPH_SAMPLES)
    {
        _delays.pop_front();
        _diff.pop_front();
    }
}

void screenxy::start()
{

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0, 0.0, 0.0, 1.0);

    glViewport(0, 0, WIN_GL_W, 600);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_GL_W, 600, 0);
    glMatrixMode(GL_MODELVIEW);

}

void screenxy::pc(bool force, int y, const char *format, ...)
{
    char out[256]={0};
    if(_m.try_lock())
    {
        va_list args;
        va_start(args, format);
        ::vsnprintf(out, sizeof(out)-4, format, args);
        va_end(args);
        _texts[y]=out;
        _m.unlock();
    }
    start();
    glPushMatrix();
        glLoadIdentity();
        glTranslatef(0.0f, 500, 0.0f);
        glScalef(float(WIN_GL_W)/float(GRAPH_SAMPLES),-(400.0f/_max),1.0f);

        glColor3f(1.0f,0.0f,1.0f);
        glBegin(GL_LINE_STRIP);
            float xp=0.0f;
            for(const auto& a : _delays)
            {
                glVertex2f(xp,a+10);
                xp+=1.0f;
            }
        glEnd();
    glPopMatrix();

    glPushMatrix();
        glLoadIdentity();
        glTranslatef(0.0f, 300, 0.0f);
        glScalef(float(WIN_GL_W)/float(GRAPH_SAMPLES),-1.0,1.0f);



        glColor3f(0.0f,0.4f,0.4f);
        glBegin(GL_LINES);
        for(int line=-40;line<40;line++){
            glVertex2f(0,line*FRM_SCALE);
            glVertex2f(GRAPH_SAMPLES,line*FRM_SCALE);
        }
        glEnd();
        char xout[32];
        int ql=0;
        for(int line=-50;line<50;line++){
            glRasterPos2f(0, line*FRM_SCALE);
            sprintf(xout,"%d",ql++);
            glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char*)xout);
        }
        glColor3f(1.0f,1.0f,1.0f);
        glBegin(GL_LINES);
        glVertex2f(0,0);
        glVertex2f(GRAPH_SAMPLES,0);
        glEnd();
        glColor3f(1.0f,1.0f,0.0f);
        glBegin(GL_LINE_STRIP);
            xp=0.0f;
            for(const auto& a : _diff)
            {
                glVertex2f(xp,a);
                xp+=1.0f;
            }
        glEnd();
    glPopMatrix();

    glPushMatrix();
        glColor3f(1.0f,1.0f,1.0f);
        for(int i = 0; i <64; i++){
            if(_texts[i].empty()) continue;
            glRasterPos2f(0, 10+i*12);
            glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char*)_texts[i].c_str());
        }
    glPopMatrix();
    swap();

}

void screenxy::swap()
{
    glutSwapBuffers();
}
#else
void screenxy::pc(bool force, int y, const char *format, ...)
{

    (void)(force);
    if(_m.try_lock())
    {
        char out[256]={0};
        va_list args;
        va_start(args, format);
        ::vsnprintf(out, sizeof(out)-4, format, args);
        va_end(args);

        mvaddstr(y, 0, out);
        ::wredrawln(_pw,y,1);
        //::redrawwin(_pw);
        ::refresh();
        _m.unlock();
    }
}
#endif
