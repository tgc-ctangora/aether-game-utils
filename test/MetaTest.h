//------------------------------------------------------------------------------
// MetaTest.h
//------------------------------------------------------------------------------
// Copyright (c) 2020 John Hughes
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//------------------------------------------------------------------------------
#ifndef METATEST_H
#define METATEST_H

//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "aeMeta.h"

//------------------------------------------------------------------------------
// TestEnumClass
//------------------------------------------------------------------------------
AE_ENUM_DECLARE( TestEnumClass, int32_t,
  NegativeOne = -1,
  Zero,
  One,
  Two = 0x02,
  Three = 0x03,
  Four,
  Five
);

//------------------------------------------------------------------------------
// PlayerState
//------------------------------------------------------------------------------
AE_ENUM_DECLARE( PlayerState, uint16_t,
  Idle,
  Run,
  Jump
);

//------------------------------------------------------------------------------
// SomeClass
//------------------------------------------------------------------------------
class SomeClass : public aeInheritor< aeObject, SomeClass >
{
public:
  int32_t intMember;
  TestEnumClass enumTest;
};

//------------------------------------------------------------------------------
// SomeOldEnum
//------------------------------------------------------------------------------
enum SomeOldEnum
{
  Bleep = 4,
  Bloop,
  Blop = 7
};

//------------------------------------------------------------------------------
// SomeOldPrefixEnum
//------------------------------------------------------------------------------
enum SomeOldPrefixEnum
{
  kSomeOldPrefixEnum_Bleep = 4,
  kSomeOldPrefixEnum_Bloop,
  kSomeOldPrefixEnum_Blop = 7
};

//------------------------------------------------------------------------------
// SomeOldRenamedEnum
//------------------------------------------------------------------------------
enum SomeOldRenamedEnum
{
  BLEEP = 4,
  BLOOP,
  BLOP = 7
};

//------------------------------------------------------------------------------
// SomeNewEnum
//------------------------------------------------------------------------------
enum class SomeNewEnum
{
  Bleep = 4,
  Bloop,
  Blop = 7
};

//------------------------------------------------------------------------------
// A::B::SomeNewEnum
//------------------------------------------------------------------------------
namespace A
{
  namespace B
  {
    enum class SomeNewEnum
    {
      Bleep = 4,
      Bloop,
      Blop = 7
    };
  }
}

#endif
