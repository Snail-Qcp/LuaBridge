//==============================================================================
/*
  Copyright (C) 2012, Vinnie Falco <vinnie.falco@gmail.com>
  Copyright (C) 2007, Nathan Reed

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
//==============================================================================

#include "luabridge.h"
#include <cstdio>

#ifdef _MSC_VER
#  if (_MSC_VER >= 1400)
#    define snprintf _snprintf_s
#  else
#    define snprintf _snprintf
# endif
#endif

using namespace std;
using namespace luabridge;

/*
* Default name for unknown types
*/

const char *luabridge::classname_unknown = "(unknown type)";


/*
* Class type checker.  Given the index of a userdata on the stack, makes
* sure that it's an object of the given classname or a subclass thereof.
* If yes, returns the address of the data; otherwise, throws an error.
* Works like the luaL_checkudata function.
*/

void *luabridge::checkclass (lua_State *L, int idx, const char *tname,
  bool exact)
{
  // If idx is relative to the top of the stack, convert it into an index
  // relative to the bottom of the stack, so we can push our own stuff
  if (idx < 0)
    idx += lua_gettop(L) + 1;

  // Check that the thing on the stack is indeed a userdata
  if (!lua_isuserdata(L, idx))
    util::typeError (L, idx, tname);

  // Lookup the given name in the registry
  luaL_getmetatable(L, tname);

  // Lookup the metatable of the given userdata
  lua_getmetatable(L, idx);

  // If exact match required, simply test for identity.
  if (exact)
  {
    // Ignore "const" for exact tests (which are used for destructors).
    if (!strncmp(tname, "const ", 6))
      tname += 6;

    if (lua_rawequal(L, -1, -2))
      return lua_touserdata(L, idx);
    else
    {
      // Generate an informative error message
      rawgetfield(L, -1, "__type");
      char buffer[256];
      snprintf(buffer, 256, "%s expected, got %s", tname,
        lua_tostring(L, -1));
      // luaL_argerror does not return
      luaL_argerror(L, idx, buffer);
      return 0;
    }
  }

  // Navigate up the chain of parents if necessary
  while (!lua_rawequal(L, -1, -2))
  {
    // Check for equality to the const metatable
    rawgetfield(L, -1, "__const");
    if (!lua_isnil(L, -1))
    {
      if (lua_rawequal(L, -1, -3))
        break;
    }
    lua_pop(L, 1);

    // Look for the metatable's parent field
    rawgetfield(L, -1, "__parent");

    // No parent field?  We've failed; generate appropriate error
    if (lua_isnil(L, -1))
    {
      // Lookup the __type field of the original metatable, so we can
      // generate an informative error message
      lua_getmetatable(L, idx);
      rawgetfield(L, -1, "__type");

      char buffer[256];
      snprintf(buffer, 256, "%s expected, got %s", tname,
        lua_tostring(L, -1));
      // luaL_argerror does not return
      luaL_argerror(L, idx, buffer);
      return 0;
    }

    // Remove the old metatable from the stack
    lua_remove(L, -2);
  }

  // Found a matching metatable; return the userdata
  return lua_touserdata(L, idx);
}


