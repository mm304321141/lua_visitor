
#include <iostream>
#include <cassert>
#include <tuple>
#include <functional>

extern "C"
{
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "lua/lundump.h"
}




namespace LuaVisitor
{
    
    template<class T> void LuaPushObj(lua_State *L, T const &t)
    {
        assert(false);
    }
    template<> void LuaPushObj(lua_State *L, int const &t)
    {
        lua_pushinteger(L, t);
    }
    template<> void LuaPushObj(lua_State *L, long long const &t)
    {
        lua_pushinteger(L, ptrdiff_t(t));
    }
    template<> void LuaPushObj(lua_State *L, double const &t)
    {
        lua_pushnumber(L, t);
    }
    template<> void LuaPushObj(lua_State *L, char const *const &t)
    {
        lua_pushstring(L, t);
    }
    template<> void LuaPushObj(lua_State *L, std::string const &t)
    {
        lua_pushstring(L, t.c_str());
    }
    template<> void LuaPushObj(lua_State *L, bool const &t)
    {
        lua_pushboolean(L, t);
    }
    
    
    template<class T> T LuaGetObj(lua_State *L, int index)
    {
        assert(false);
    }
    template<> int LuaGetObj(lua_State *L, int index)
    {
        return int(lua_tointeger(L, index));
    }
    template<> long long LuaGetObj(lua_State *L, int index)
    {
        return (long long)(lua_tointeger(L, index));
    }
    template<> double LuaGetObj(lua_State *L, int index)
    {
        return double(lua_tonumber(L, index));
    }
    template<> float LuaGetObj(lua_State *L, int index)
    {
        return float(lua_tonumber(L, index));
    }
    template<> char const *LuaGetObj(lua_State *L, int index)
    {
        return lua_tostring(L, index);
    }
    template<> std::string LuaGetObj(lua_State *L, int index)
    {
        return lua_tostring(L, index);
    }
    template<> bool LuaGetObj(lua_State *L, int index)
    {
        return lua_toboolean(L, index);
    }

    
    
    template<size_t ...I>
    struct LuaIndexes
    {
    };
    template<size_t S, size_t ...I>
    struct LuaMakeIndexes
    {
        typedef typename LuaMakeIndexes<S - 1, I..., sizeof...(I)>::indexes indexes;
    };
    template<size_t ... I>
    struct LuaMakeIndexes<0, I...>
    {
        typedef LuaIndexes<I...> indexes;
    };
    
    template<class ...P, size_t ...I> int LuaMultiRuturn(lua_State *L, std::tuple<P...> &&result, LuaIndexes<I...>)
    {
        std::initializer_list<int>({ (LuaPushObj(L, std::get<I>(result)), 0)... });
        return sizeof...(P);
    }
    template<class R> int LuaRuturn(lua_State *L, R &&result)
    {
        LuaPushObj(L, std::forward<R>(result));
        return 1;
    }
    template<class ...P> int LuaRuturn(lua_State *L, std::tuple<P...> &&result)
    {
        typedef typename LuaMakeIndexes<sizeof...(P)>::indexes indexes;
        return LuaMultiRuturn(L, std::move(result), indexes());
    }
    
    template<class ...P>
    struct LuaResult
    {
        typedef std::tuple<typename std::decay<P>::type...> type;
    };
    template<class ...P>
    struct LuaResult<LuaResult<P...>>
    {
        typedef typename LuaResult<P...>::type type;
    };
    template<class T>
    struct LuaResult<T>
    {
        typedef typename std::decay<T>::type type;
    };
    
    template<class R, class ... P, size_t ...I>
    int LuaCallBack(lua_State *L, LuaIndexes<I...>, R (*f)(P...))
    {
        return LuaRuturn(L, f(LuaGetObj<P>(L, I + 1)...));
    }
    template<class R, class ... P, size_t ...I>
    int LuaCallBack(lua_State *L, LuaIndexes<I...>, std::function<R(P...)> const &f)
    {
        return LuaRuturn(L, f(LuaGetObj<P>(L, I + 1)...));
    }
    template<class ... P, size_t ...I>
    int LuaCallBack(lua_State *L, LuaIndexes<I...>, void (*f)(P...))
    {
        f(LuaGetObj<P>(L, I + 1)...);
        return 0;
    }
    template<class ... P, size_t ...I>
    int LuaCallBack(lua_State *L, LuaIndexes<I...>, std::function<void(P...)> const &f)
    {
        f(LuaGetObj<P>(L, I + 1)...);
        return 0;
    }
    
    template<class ...P>
    std::tuple<typename std::decay<P>::type...> LuaTable(P &&...p)
    {
        return std::tuple<typename std::decay<P>::type...>(std::forward<P>(p)...);
    };
    template<class T> void LuaGetTable(lua_State *L, T &&t)
    {
        LuaPushObj(L, std::forward<T>(t));
        lua_gettable(L, -2);
    }
    
    template<class ...P, size_t ...I> void LuaCallGetTable(lua_State *L, LuaIndexes<I...>, std::tuple<P...> const &table)
    {
        std::initializer_list<int>({ (LuaGetTable(L, std::get<I + 1>(table)), 0)... });
    }
    template<class ...P, size_t ...I> std::tuple<P...> LuaCallGetResult(lua_State *L, LuaIndexes<I...>)
    {
        return {LuaGetObj<P>(L, int(I) - int(sizeof...(P)))...};
    }
    
    template<class T, int N, int M> struct LuaGetResult
    {
        T operator()(lua_State *L)
        {
            lua_call(L, N, 1);
            auto result = LuaGetObj<T>(L, -1);
            lua_pop(L, M + 1);
            return result;
        }
    };
    template<class ...P, int N, int M> struct LuaGetResult<LuaResult<P...>, N, M>
    {
        std::tuple<typename std::decay<P>::type...> operator()(lua_State *L)
        {
            typedef typename LuaMakeIndexes<sizeof...(P)>::indexes indexes;
            lua_call(L, N, sizeof...(P));
            auto result = LuaCallGetResult<typename std::decay<P>::type...>(L, indexes());
            lua_pop(L, M + int(sizeof...(P)));
            return result;
        }
    };
    template<int N, int M> struct LuaGetResult<void, N, M>
    {
        void operator()(lua_State *L)
        {
            lua_call(L, N, 0);
            if(M > 0)
            {
                lua_pop(L, M);
            }
        }
    };
    
    struct LuaObject
    {
        void *obj;
        char const *name;
    };
    
    
    template<class R, class ...P>
    typename LuaResult<R>::type LuaCallFunction(lua_State *L, char const *func, P &&...p)
    {
        lua_getglobal(L, func);
        std::initializer_list<int>({ (LuaPushObj(L, std::forward<P>(p)), 0)... });
        return LuaGetResult<R, sizeof...(P), 0>()(L);
    }
    template<class R, class ...P, class ...T>
    typename LuaResult<R>::type LuaCallFunction(lua_State *L, std::tuple<T...> const &func, P &&...p)
    {
        typedef typename LuaMakeIndexes<sizeof...(T) - 1>::indexes indexes;
        lua_getglobal(L, std::get<0>(func));
        LuaCallGetTable(L, indexes(), func);
        std::initializer_list<int>({ (LuaPushObj(L, std::forward<P>(p)), 0)... });
        return LuaGetResult<R, sizeof...(P), sizeof...(T) - 1>()(L);
    }
    template<class R, class ...P>
    void LuaRegFunction(lua_State *L, char const *func, R (*f)(P...))
    {
        struct handler
        {
            static int invoke(lua_State *L)
            {
                typedef typename LuaMakeIndexes<sizeof...(P)>::indexes indexes;
                R (*callback)(P...) = reinterpret_cast<R (*)(P...)>(lua_touserdata(L, lua_upvalueindex(1)));
                return LuaCallBack(L, indexes(), callback);
            }
        };
        lua_pushlightuserdata(L, reinterpret_cast<void *>(f));
        lua_pushcclosure(L, handler::invoke, 1);
        lua_setglobal(L, func);
    }
    
}

using LuaVisitor::LuaTable;
using LuaVisitor::LuaResult;
using LuaVisitor::LuaRegFunction;
using LuaVisitor::LuaCallFunction;


int foo(int a, int b)
{
    return a + b;
}
std::tuple<int, int> bar(int a, int b, int c)
{
    return {a + b + c + 100, 100};
}
int call_count = 0;
void asd()
{
    ++call_count;
}


void test_lua(int count)
{
    lua_State *L = luaL_newstate();
    luaopen_base(L);
    luaopen_table(L);
    luaopen_io(L);
    luaopen_string(L);
    luaopen_math(L);
    
    LuaRegFunction(L, "foo", foo);
    LuaRegFunction(L, "bar", bar);
    LuaRegFunction(L, "asd", asd);
    
    luaL_dostring(L, R"(
        ttt = {};
        ttt[1] = {};
        local ccc = ttt[1];
        ccc.run = function(num)
            return 1, num, "foo";
        end
        
        function test(count)
            local sum = 0, i;
            for i = 0, count - 1, 1 do
                sum = sum + foo(0, i);
                local r1, r2 = bar(1, i, -1);
                sum = sum + r1 - r2;
                asd();
            end
            return sum;
        end
    )");
    
    LuaCallFunction<void>(L, "test", 10);
    std::cout << LuaCallFunction<long long>(L, "test", count) << std::endl;
    auto ret = LuaCallFunction<LuaResult<int, int, std::string>>(L, LuaTable("ttt", 1, "run"), 666);
    std::cout << std::get<0>(ret) << std::endl;
    std::cout << std::get<1>(ret) << std::endl;
    std::cout << std::get<2>(ret) << std::endl;

    std::cout << call_count << std::endl;
    
    lua_close(L);
}

struct scope_timer
{
    std::string msg;
    std::chrono::high_resolution_clock::time_point pt;
    template<class T>
    scope_timer(T &&t) : msg(std::forward<T>(t)), pt(std::chrono::high_resolution_clock::now())
    {
    }
    ~scope_timer()
    {
        using namespace std::chrono;
        std::cout
            << msg
            << " "
            << duration_cast<duration<float, std::milli>>(high_resolution_clock::now() - pt).count()
            << std::endl;
    }
};

int main()
{
    scope_timer st("lua");
    test_lua(1000000);
    return 0;
}

