#pragma once

#include "WebSocketServer.h"
#include "boost/variant.hpp"
#include "boost/signals2.hpp"
#include <map>

class WebUI
{
public:
    WebUI();

    void                        update();
    void                        listen( uint16_t port );

private:
    WebSocketServer             mServer;

    void                        write( const std::string &msg );

    void						onConnect();
    void						onDisconnect();
    void						onError( std::string err );
    void						onInterrupt();
    void						onPing( std::string msg );
    void						onRead( std::string msg );

protected:
    class Event
    {
    public:
        enum class Type { UNKNOWN, SET };
        Event();
        Event( const Type &type, const std::string &name, const std::string &value );

        Type                    getType() const { return mType; }
        std::string             getName() const { return mName; }
        std::string             getValue() const { return mValue; }

        operator bool() const { return mIsValid; }
    private:
        bool                    mIsValid;
        Type                    mType;
        std::string             mName, mValue;
    };

private:
    typedef boost::signals2::signal< void ( Event ) > EventSignal;
    EventSignal                 mSetSignal;

protected:
    EventSignal &               getSetSignal() { return mSetSignal; }
    
private:

    Event                       parse( const std::string &msg );
    void                        dispatch( const Event &event );
};

class WebParamUI : public WebUI
{
public:
    WebParamUI();

    class ParamOptions
    {

    };

    class Param
    {
    public:
        Param( const std::string &name, float *ptr );

        ParamOptions &              getOptions();

        void                        setFromString( const std::string &string );
        
        typedef boost::variant< float * > ptr_t;
    private:
        std::string                 mName;
        ParamOptions                mOptions;
        ptr_t                       mPtr;
    };
    typedef std::map< std::string, Param > ParamContainer;

    ParamOptions &                  addParam( const std::string &name, float *floatParam );
    Param &                         getParam( const std::string &name );
    ParamContainer::iterator        findParam( const std::string &name );

private:
    void                            onSet( Event event );

    ParamContainer                  mParams;

};