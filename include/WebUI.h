#pragma once

#include "WebSocketServer.h"
#include "cinder/Json.h"
#include "boost/variant.hpp"
#include "boost/signals2.hpp"
#include <map>

namespace webui {

////////////////////////////////////////////////////////////////////////////////
// Event
class Event
{
public:
    enum class Type { UNKNOWN, GET, SET };
    Event();
    Event( const Type &type, const ci::JsonTree &data );

    Type                    getType() const { return mType; }
    const ci::JsonTree &    getData() const { return mData; }
    ci::JsonTree            getData() { return mData; }
private:
    Type                    mType;
    ci::JsonTree            mData;
};

////////////////////////////////////////////////////////////////////////////////
// Server
class Server : public WebSocketServer
{
public:
    typedef boost::signals2::signal< void ( Event ) > EventSignal;
    typedef std::map< Event::Type, EventSignal > EventSignalContainer;

    Server();

    EventSignal &               getEventSignal( const Event::Type &type );

    void                        get( const std::string &name );
    template< typename T >
    void                        set( const std::string &name, const T &value );

private:
    void						onConnect();
    void						onDisconnect();
    void						onError( std::string err );
    void						onInterrupt();
    void						onPing( std::string msg );
    void						onRead( std::string msg );

    EventSignalContainer        mEventSignals;
    void                        dispatch( const Event &event );
};

////////////////////////////////////////////////////////////////////////////////
// BaseUI
class BaseUI
{
public:
    BaseUI();

    void                        update();
    void                        listen( uint16_t port );

protected:
    Server                      mServer;
};

////////////////////////////////////////////////////////////////////////////////
// ParamUI
class ParamUI : public BaseUI
{
public:
    ParamUI();

    struct Param
    {
        Param( const std::string &name, float *ptr );
        typedef boost::variant< float * > ptr_t;
        std::string                 mName;
        ptr_t                       mPtr;
    };
    typedef std::map< std::string, Param > ParamContainer;

    template< typename T >
    void                            bind( const std::string &name, T *param )
    {
        mParams.emplace( name, Param( name, param ) );
    }

    ParamContainer::iterator        findParam( const std::string &name );

private:
    void                            onSet( Event event );
    void                            onGet( Event event );

    ParamContainer                  mParams;

};

} // end namespace webui