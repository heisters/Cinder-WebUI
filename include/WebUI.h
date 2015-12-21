#pragma once

#include "cinder/app/App.h"

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
    enum class Type { UNKNOWN, GET, SET, SELECT };
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
    typedef boost::signals2::signal< void ( Event ) > event_signal;
    typedef std::map< Event::Type, event_signal > event_signal_container;

    Server();

    event_signal &              getEventSignal( const Event::Type &type );

    void                        get( const std::string &name );
    template< typename T >
    void                        set( const std::string &name, const T &value );
    void                        set( const std::string &name, const glm::vec2 &value );
    void                        set( const std::string &name, const glm::vec3 &value );
    void                        set( const std::string &name, const ci::Colorf &value );
    template< typename T >
    void                        set( const std::string &name, const std::vector< T > &value );
    template< typename T, typename U >
    void                        set( const std::string &name, const std::map< T, U > &value );
    void                        dispatch( const Event &event );

private:
    void						onConnect();
    void						onDisconnect();
    void						onError( std::string err );
    void						onInterrupt();
    void						onPing( std::string msg );
    void						onRead( std::string msg );

    event_signal_container      mEventSignals;
};

////////////////////////////////////////////////////////////////////////////////
// BoundParam
enum source { local, remote };

template< typename T > struct contained_type { typedef T value; };
template< typename U > struct contained_type< std::vector< U > > { typedef U value; };
template< typename V > struct contained_type< std::map< V, V > > { typedef std::pair< V, V > value; };

template< typename T >
class BoundParam
{
public:
    typedef T value;
    typedef typename contained_type< T >::value contained_value;
    typedef boost::signals2::signal< void ( source, value ) > signal;
    typedef boost::signals2::signal< void ( source, contained_value ) > signal_select;


    BoundParam() {};
    BoundParam( const T &v ) : mValue( v ) {};

    // no "T&" methods are provided, so that you can't accidentally modify the
    // value without notifying of set

    T                           get() const { return mValue; }
    contained_value             selected() const { return mSelected; }
    T                           set( const T & v, const source s=local ) { mValue = v; notifySet( s ); return get(); }
    template< typename U = T >
    void                        set( const typename U::value_type &v, const source s=local ) { mValue[v.first] = v.second; notifySet( s ); }
    contained_value             select( const contained_value &v, const source s=local ) { mSelected = v; notifySelect( s ); return selected(); }

    template< typename U = T >
    void                        push_back( const typename U::value_type &v, const source s=local ) { mValue.push_back( v ); notifySet( s ); }
    template< typename U = T >
    void                        clear( const source s=local ) { mValue.clear(); notifySet( s ); }

    
    operator                    T () const { return get(); }
    T operator                  () () const { return get(); }
    T operator                  = ( const T & v ) { return set( v ); }
    T operator                  += ( const T & v ) { return set( mValue + v ); }
    T operator                  -= ( const T & v ) { return set( mValue - v ); }
    T operator                  ++ () { mValue++; notifySet( local ); return get(); }
    T operator                  -- () { mValue--; notifySet( local ); return get(); }

    signal &                    getSetSignal() { return mLocalSetSignal; }
    signal_select &             getSelectSignal() { return mLocalSelectSignal; }

private:
    void                        notifySet( const source s ) { getSetSignal()( s, get() ); }
    void                        notifySelect( const source s ) { getSelectSignal()( s, selected() ); }

    T                           mValue;
    contained_value             mSelected;
    signal                      mLocalSetSignal;
    signal_select               mLocalSelectSignal;
};

////////////////////////////////////////////////////////////////////////////////
// WebUI
class WebUI
{
public:
    WebUI();

    void                            update();
    void                            listen( uint16_t port );

    typedef boost::variant< BoundParam< bool >*, BoundParam< int >*, BoundParam< float >*, BoundParam< glm::vec2 >*, BoundParam< glm::vec3 >*, BoundParam< std::string >*, BoundParam< double >*, BoundParam< ci::Colorf >*, BoundParam< std::vector< std::string > >*, BoundParam< std::map< std::string, std::string > >* > bound_param_ptr;
    typedef std::map< std::string, bound_param_ptr > bound_params_container;

    template< typename T >
    void                            bind( const std::string &name, T *param )
    {
        param->getSetSignal().connect( std::bind( &WebUI::onLocalSet, this, std::placeholders::_1, name ) );
        mParams.insert( make_pair( name, param ) );
    }

private:

    bound_params_container::iterator findParam( const std::string &name );
    void                            onRemoteSet( Event event );
    void                            onRemoteGet( Event event );
    void                            onRemoteSelect( Event event );
    void                            onLocalSet( source s, const std::string &name );

    bound_params_container          mParams;

    Server                          mServer;
};

} // end namespace webui