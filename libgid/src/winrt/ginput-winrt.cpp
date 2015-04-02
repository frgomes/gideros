#include <ginput.h>
#include <ginput-winrt.h>
#include <vector>
#include <gevent.h>
#include <memory.h>
#include <string.h>
#include <pthread.h>
#include <map>

namespace ginput {

class InputManager
{
public:
    InputManager()
    {
        isMouseToTouchEnabled_ = 0;
        isTouchToMouseEnabled_ = 0;
        mouseTouchOrder_= 0;

        keyMap_[GINPUT_KEY_LEFT] = GINPUT_KEY_LEFT;
        keyMap_[GINPUT_KEY_RIGHT] = GINPUT_KEY_RIGHT;
        keyMap_[GINPUT_KEY_UP] = GINPUT_KEY_UP;
        keyMap_[GINPUT_KEY_DOWN] = GINPUT_KEY_DOWN;

        keyMap_[GINPUT_KEY_0] = GINPUT_KEY_0;
        keyMap_[GINPUT_KEY_1] = GINPUT_KEY_1;
        keyMap_[GINPUT_KEY_2] = GINPUT_KEY_2;
        keyMap_[GINPUT_KEY_3] = GINPUT_KEY_3;
        keyMap_[GINPUT_KEY_4] = GINPUT_KEY_4;
        keyMap_[GINPUT_KEY_5] = GINPUT_KEY_5;
        keyMap_[GINPUT_KEY_6] = GINPUT_KEY_6;
        keyMap_[GINPUT_KEY_7] = GINPUT_KEY_7;
        keyMap_[GINPUT_KEY_8] = GINPUT_KEY_8;
        keyMap_[GINPUT_KEY_9] = GINPUT_KEY_9;

        keyMap_[GINPUT_KEY_A] = GINPUT_KEY_A;
        keyMap_[GINPUT_KEY_B] = GINPUT_KEY_B;
        keyMap_[GINPUT_KEY_C] = GINPUT_KEY_C;
        keyMap_[GINPUT_KEY_D] = GINPUT_KEY_D;
        keyMap_[GINPUT_KEY_E] = GINPUT_KEY_E;
        keyMap_[GINPUT_KEY_F] = GINPUT_KEY_F;
        keyMap_[GINPUT_KEY_G] = GINPUT_KEY_G;
        keyMap_[GINPUT_KEY_H] = GINPUT_KEY_H;
        keyMap_[GINPUT_KEY_I] = GINPUT_KEY_I;
        keyMap_[GINPUT_KEY_J] = GINPUT_KEY_J;
        keyMap_[GINPUT_KEY_K] = GINPUT_KEY_K;
        keyMap_[GINPUT_KEY_L] = GINPUT_KEY_L;
        keyMap_[GINPUT_KEY_M] = GINPUT_KEY_M;
        keyMap_[GINPUT_KEY_N] = GINPUT_KEY_N;
        keyMap_[GINPUT_KEY_O] = GINPUT_KEY_O;
        keyMap_[GINPUT_KEY_P] = GINPUT_KEY_P;
        keyMap_[GINPUT_KEY_Q] = GINPUT_KEY_Q;
        keyMap_[GINPUT_KEY_R] = GINPUT_KEY_R;
        keyMap_[GINPUT_KEY_S] = GINPUT_KEY_S;
        keyMap_[GINPUT_KEY_T] = GINPUT_KEY_T;
        keyMap_[GINPUT_KEY_U] = GINPUT_KEY_U;
        keyMap_[GINPUT_KEY_V] = GINPUT_KEY_V;
        keyMap_[GINPUT_KEY_W] = GINPUT_KEY_W;
        keyMap_[GINPUT_KEY_X] = GINPUT_KEY_X;
        keyMap_[GINPUT_KEY_Y] = GINPUT_KEY_Y;
        keyMap_[GINPUT_KEY_Z] = GINPUT_KEY_Z;

        pthread_mutex_init(&touchPoolMutex_, NULL);

        gevent_AddCallback(posttick_s, this);

        gid_ = g_NextId();
    }

    ~InputManager()
    {
        gevent_RemoveCallbackWithGid(gid_);

        gevent_RemoveCallback(posttick_s, this);

        for (size_t i = 0; i < mousePool1_.size(); ++i)
            delete mousePool1_[i];
        for (size_t i = 0; i < mousePool2_.size(); ++i)
            delete mousePool2_[i];

        for (size_t i = 0; i < keyPool1_.size(); ++i)
            delete keyPool1_[i];
        for (size_t i = 0; i < keyPool2_.size(); ++i)
            delete keyPool2_[i];


        pthread_mutex_lock(&touchPoolMutex_);
        std::map<size_t, std::vector<ginput_TouchEvent*> >::iterator iter;
        for (iter = touchPool1_.begin(); iter != touchPool1_.end(); ++iter)
        {
            const std::vector<ginput_TouchEvent*> &v = iter->second;
            for (size_t i = 0; i < v.size(); ++i)
            {
                delete [] v[i]->allTouches;
                delete v[i];
            }
        }
        for (iter = touchPool2_.begin(); iter != touchPool2_.end(); ++iter)
        {
            const std::vector<ginput_TouchEvent*> &v = iter->second;
            for (size_t i = 0; i < v.size(); ++i)
            {
                delete [] v[i]->allTouches;
                delete v[i];
            }
        }
        pthread_mutex_unlock(&touchPoolMutex_);

        pthread_mutex_destroy(&touchPoolMutex_);
    }

    static void posttick_s(int type, void *event, void *udata)
    {
        if (type == GEVENT_POST_TICK_EVENT)
            static_cast<InputManager*>(udata)->posttick();
    }

    void posttick()
    {
        for (size_t i = 0; i < mousePool2_.size(); ++i)
            mousePool1_.push_back(mousePool2_[i]);
        mousePool2_.clear();

        for (size_t i = 0; i < keyPool2_.size(); ++i)
            keyPool1_.push_back(keyPool2_[i]);
        keyPool2_.clear();


        pthread_mutex_lock(&touchPoolMutex_);
        std::map<size_t, std::vector<ginput_TouchEvent*> >::iterator iter;
        for (iter = touchPool2_.begin(); iter != touchPool2_.end(); ++iter)
        {
            const std::vector<ginput_TouchEvent*> &v = iter->second;
            for (size_t i = 0; i < v.size(); ++i)
                touchPool1_[iter->first].push_back(v[i]);
        }
        touchPool2_.clear();
        pthread_mutex_unlock(&touchPoolMutex_);
    }

    void mouseDown(int x, int y, int button)
    {
        ginput_MouseEvent *mouseEvent = newMouseEvent(x, y, button);

        ginput_TouchEvent *touchEvent = NULL;
        if (isMouseToTouchEnabled_)
        {
            touchEvent = newTouchEvent(1);
            touchEvent->touch.x = x;
            touchEvent->touch.y = y;
            touchEvent->touch.id = 0;
            touchEvent->allTouches[0].x = x;
            touchEvent->allTouches[0].y = y;
            touchEvent->allTouches[0].id = 0;
        }

        if (mouseTouchOrder_ == 0)
        {
            gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_DOWN_EVENT, mouseEvent, 0, this);
            deleteMouseEvent(mouseEvent);
            if (touchEvent)
            {
                gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_BEGIN_EVENT, touchEvent, 0, this);
                deleteTouchEvent(touchEvent);
            }

        }
        else
        {
            if (touchEvent)
            {
                gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_BEGIN_EVENT, touchEvent, 0, this);
                deleteTouchEvent(touchEvent);
            }
            gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_DOWN_EVENT, mouseEvent, 0, this);
            deleteMouseEvent(mouseEvent);
        }
    }

    void mouseMove(int x, int y)
    {
        ginput_MouseEvent *mouseEvent = newMouseEvent(x, y, GINPUT_NO_BUTTON);

        ginput_TouchEvent *touchEvent = NULL;
        if (isMouseToTouchEnabled_)
        {
            touchEvent = newTouchEvent(1);
            touchEvent->touch.x = x;
            touchEvent->touch.y = y;
            touchEvent->touch.id = 0;
            touchEvent->allTouches[0].x = x;
            touchEvent->allTouches[0].y = y;
            touchEvent->allTouches[0].id = 0;
        }

        if (mouseTouchOrder_ == 0)
        {
            gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_MOVE_EVENT, mouseEvent, 0, this);
            deleteMouseEvent(mouseEvent);
            if (touchEvent)
            {
                gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_MOVE_EVENT, touchEvent, 0, this);
                deleteTouchEvent(touchEvent);
            }

        }
        else
        {
            if (touchEvent)
            {
                gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_MOVE_EVENT, touchEvent, 0, this);
                deleteTouchEvent(touchEvent);
            }
            gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_MOVE_EVENT, mouseEvent, 0, this);
            deleteMouseEvent(mouseEvent);
        }
    }

    void mouseUp(int x, int y, int button)
    {
        ginput_MouseEvent *mouseEvent = newMouseEvent(x, y, button);

        ginput_TouchEvent *touchEvent = NULL;
        if (isMouseToTouchEnabled_)
        {
            touchEvent = newTouchEvent(1);
            touchEvent->touch.x = x;
            touchEvent->touch.y = y;
            touchEvent->touch.id = 0;
            touchEvent->allTouches[0].x = x;
            touchEvent->allTouches[0].y = y;
            touchEvent->allTouches[0].id = 0;
        }

        if (mouseTouchOrder_ == 0)
        {
            gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_UP_EVENT, mouseEvent, 0, this);
            deleteMouseEvent(mouseEvent);
            if (touchEvent)
            {
                gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_END_EVENT, touchEvent, 0, this);
                deleteTouchEvent(touchEvent);
            }

        }
        else
        {
            if (touchEvent)
            {
                gevent_EnqueueEvent(gid_, callback_s, GINPUT_TOUCH_END_EVENT, touchEvent, 0, this);
                deleteTouchEvent(touchEvent);
            }
            gevent_EnqueueEvent(gid_, callback_s, GINPUT_MOUSE_UP_EVENT, mouseEvent, 0, this);
            deleteMouseEvent(mouseEvent);
        }
    }

    void keyDown(int keyCode)
    {
        keyCode = convertKeyCode(keyCode);
        if (keyCode == 0)
            return;

        ginput_KeyEvent *event = newKeyEvent(keyCode);
        gevent_EnqueueEvent(gid_, callback_s, GINPUT_KEY_DOWN_EVENT, event, 0, this);
        deleteKeyEvent(event);
    }

    void keyUp(int keyCode)
    {
        keyCode = convertKeyCode(keyCode);
        if (keyCode == 0)
            return;

        ginput_KeyEvent *event = newKeyEvent(keyCode);
        gevent_EnqueueEvent(gid_, callback_s, GINPUT_KEY_UP_EVENT, event, 0, this);
        deleteKeyEvent(event);
    }

    void setMouseToTouchEnabled(int enabled)
    {
        isMouseToTouchEnabled_ = enabled;
    }

    void setTouchToMouseEnabled(int enabled)
    {
        isTouchToMouseEnabled_ = enabled;
    }

    void setMouseTouchOrder(int order)
    {
        mouseTouchOrder_ = order;
    }

private:
    ginput_MouseEvent *newMouseEvent(int x, int y, int button)
    {
        ginput_MouseEvent *event;

        if (mousePool1_.empty())
        {
            event = new ginput_MouseEvent;
        }
        else
        {
            event = mousePool1_.back();
            mousePool1_.pop_back();
        }

        event->x = x;
        event->y = y;
        event->button = button;

        return event;
    }

    void deleteMouseEvent(ginput_MouseEvent *event)
    {
        mousePool2_.push_back(event);
    }

    ginput_KeyEvent *newKeyEvent(int keyCode)
    {
        ginput_KeyEvent *event;

        if (keyPool1_.empty())
        {
            event = new ginput_KeyEvent;
        }
        else
        {
            event = keyPool1_.back();
            keyPool1_.pop_back();
        }

        event->keyCode = keyCode;

        return event;
    }

    void deleteKeyEvent(ginput_KeyEvent *event)
    {
        keyPool2_.push_back(event);
    }

    int convertKeyCode(int keyCode)
    {
        std::map<int, int>::const_iterator iter = keyMap_.find(keyCode);

        if (iter == keyMap_.end())
            return 0;

        return iter->second;
    }

    ginput_TouchEvent *newTouchEvent(size_t allTouchesCount)
    {
        pthread_mutex_lock(&touchPoolMutex_);
        std::vector<ginput_TouchEvent*> &pool = touchPool1_[allTouchesCount];

        ginput_TouchEvent *event;

        if (pool.empty())
        {
            event = new ginput_TouchEvent;
            event->allTouches = new ginput_Touch[allTouchesCount];
        }
        else
        {
            event = pool.back();
            pool.pop_back();
        }
        pthread_mutex_unlock(&touchPoolMutex_);

        event->allTouchesCount = allTouchesCount;

        return event;
    }

    void deleteTouchEvent(ginput_TouchEvent *event)
    {
        pthread_mutex_lock(&touchPoolMutex_);
        touchPool2_[event->allTouchesCount].push_back(event);
        pthread_mutex_unlock(&touchPoolMutex_);
    }

private:
    std::vector<ginput_MouseEvent*> mousePool1_;
    std::vector<ginput_MouseEvent*> mousePool2_;
    std::vector<ginput_KeyEvent*> keyPool1_;
    std::vector<ginput_KeyEvent*> keyPool2_;
    std::map<size_t, std::vector<ginput_TouchEvent*> > touchPool1_;
    std::map<size_t, std::vector<ginput_TouchEvent*> > touchPool2_;

    std::map<int, int> keyMap_;

    pthread_mutex_t touchPoolMutex_;

    int isMouseToTouchEnabled_;
    int isTouchToMouseEnabled_;
    int mouseTouchOrder_;

public:
    g_id addCallback(gevent_Callback callback, void *udata)
    {
        return callbackList_.addCallback(callback, udata);
    }

    void removeCallback(gevent_Callback callback, void *udata)
    {
        callbackList_.removeCallback(callback, udata);
    }

    void removeCallbackWithGid(g_id gid)
    {
        callbackList_.removeCallbackWithGid(gid);
    }

    static void callback_s(int type, void *event, void *udata)
    {
        static_cast<InputManager*>(udata)->callbackList_.dispatchEvent(type, event);
    }

private:
    gevent_CallbackList callbackList_;
    g_id gid_;
};

}

using namespace ginput;

static InputManager *s_manager = NULL;

extern "C" {

void ginput_init()
{
    s_manager = new InputManager;
}

void ginput_cleanup()
{
    delete s_manager;
    s_manager = NULL;
}

int ginput_isAccelerometerAvailable()
{
    return 0;
}

void ginput_startAccelerometer()
{

}

void ginput_stopAccelerometer()
{

}

void ginput_getAcceleration(double *x, double *y, double *z)
{
    if (x)
        *x = 0;
    if (y)
        *y = 0;
    if (z)
        *z = 0;
}

int ginput_isGyroscopeAvailable()
{
    return 0;
}

void ginput_startGyroscope()
{

}

void ginput_stopGyroscope()
{

}

void ginput_getGyroscopeRotationRate(double *x, double *y, double *z)
{
    if (x)
        *x = 0;
    if (y)
        *y = 0;
    if (z)
        *z = 0;
}

void ginputp_mouseDown(int x, int y, int button)
{
    if (s_manager)
        s_manager->mouseDown(x, y, button);
}

void ginputp_mouseMove(int x, int y)
{
    if (s_manager)
        s_manager->mouseMove(x, y);
}

void ginputp_mouseUp(int x, int y, int button)
{
    if (s_manager)
        s_manager->mouseUp(x, y, button);
}

void ginputp_keyDown(int keyCode)
{
    if (s_manager)
        s_manager->keyDown(keyCode);
}

void ginputp_keyUp(int keyCode)
{
    if (s_manager)
        s_manager->keyUp(keyCode);
}

void ginput_setMouseToTouchEnabled(int enabled)
{
    s_manager->setMouseToTouchEnabled(enabled);
}

void ginput_setTouchToMouseEnabled(int enabled)
{
    s_manager->setTouchToMouseEnabled(enabled);
}

void ginput_setMouseTouchOrder(int order)
{
    s_manager->setMouseTouchOrder(order);
}

g_id ginput_addCallback(gevent_Callback callback, void *udata)
{
    return s_manager->addCallback(callback, udata);
}

void ginput_removeCallback(gevent_Callback callback, void *udata)
{
    s_manager->removeCallback(callback, udata);
}

void ginput_removeCallbackWithGid(g_id gid)
{
    s_manager->removeCallbackWithGid(gid);
}

}
