#ifndef _StringFormat_h_
#define _StringFormat_h_

#include <functional>
#include <sstream>
#include <array>
#include <map>
#include <type_traits>

#include <iostream>

struct FormatInner
{
    template<typename T>
    static void writeStream( std::ostringstream &os, const typename std::remove_all_extents<typename std::decay<T>::type>::type &t )
    {
        os <<   t  ;
    }
};


template<typename ...Args>
std::string FormatI( const char *format, Args &&...args )
{
    std::ostringstream os;
    std::array<std::function<void()>, sizeof...( Args ) > fmt_fun = {{std::bind( FormatInner::writeStream<Args>, std::ref( os ), std::forward<Args>( args ) ) ...}};
    
    for( const char *p = format; *p != '\0'; ++p )
    {
        if( *p == '{' )
        {
            const char *item = p + 1;
            size_t value;
            
            for( value = 0; isdigit( *item ) ; ++item )
            {
                value = value * 10 + *item - '0';
            }
            
            if( *item == '}' && item - p > 1 )
            {
                if( value < fmt_fun.size() )
                {
                    fmt_fun[value]();
                }
                
                p = item;
            }
            else if( *item == '{' )
            {
                os << *item;
                p = item;
            }
            else
            {
                os << *p;
            }
        }
        else if( *p == '}' && ( *( p + 1 ) ) == '}' )
        {
            os << *p++ ;
        }
        else
        {
            os << *p;
        }
    }
    
    return os.str();
}

template<typename ...Args>
std::string FormatV( const char *format, Args &&...args )
{
    std::ostringstream os;
    std::array<std::function<void()>, sizeof...( Args ) > fmt_fun = {{std::bind( FormatInner::writeStream<typename Args::second_type>, std::ref( os ), std::forward<typename Args::second_type>( args.second ) ) ...}};
    int index = 0;
    std::map<std::string, size_t> fmt_fun_map = {{
            std::forward<typename Args::first_type>( args.first ),
            index++
        } ...
    };
    
    for( const char *p = format; *p != '\0'; ++p )
    {
        if( *p == '{' )
        {
            const char *item = p + 1;
            
            if( *item == '{' )
            {
                os << *item;
                p = item;
            }
            else
            {
                int value = 0;
                
                while( *item != '\0' && *item != '}' )
                {
                    if( value >= 0 )
                    {
                        if( isdigit( *item ) )
                        {
                            value = value * 10 + *item - '0';
                        }
                        else
                        {
                            value = -1;
                        }
                    }
                    
                    ++item;
                }
                
                if( *item == '}' && *( item + 1 ) != '}' && item - p > 1 )
                {
                    if( value >= 0 )
                    {
                        if( ( unsigned )value < fmt_fun.size() )
                        {
                            fmt_fun[value]();
                        }
                    }
                    else
                    {
                        std::string fun_key = std::string( p + 1, item - p - 1 ) ;
                        auto iter = fmt_fun_map.find( fun_key );
                        
                        if( iter != fmt_fun_map.end() && ( unsigned )iter->second < fmt_fun.size() )
                        {
                            fmt_fun[iter->second]();
                        }
                    }
                }
                
                p = item;
            }
        }
        else if( *p == '}' && ( *( p + 1 ) ) == '}' )
        {
            os << *p++ ;
        }
        else
        {
            os << *p;
        }
    }
    
    return os.str();
}

inline std::string FormatM( const char *format, const std::map<std::string, std::string> &mpParam )
{
    std::ostringstream os;
    
    for( const char *p = format; *p != '\0'; ++p )
    {
        if( *p == '{' )
        {
            const char *item = p + 1;
            
            if( *item == '{' )
            {
                os << *item;
                p = item;
            }
            else
            {
                while( *item != '\0' && *item != '}' )
                {
                    ++item;
                }
                
                if( *item == '}' && *( item + 1 ) != '}' && item - p > 1 )
                {
                    std::string fun_key = std::string( p + 1, item - p - 1 ) ;
                    auto iter = mpParam.find( fun_key );
                    
                    if( iter != mpParam.end() )
                    {
                        os << iter->second;
                    }
                    else
                    {
                        os << fun_key;
                    }
                }
                
                p = item;
            }
        }
        else if( *p == '}' && ( *( p + 1 ) ) == '}' )
        {
            os << *p++ ;
        }
        else
        {
            os << *p;
        }
    }
    
    return os.str();
}

#endif
