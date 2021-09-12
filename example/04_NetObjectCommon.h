//------------------------------------------------------------------------------
// 04_NetObjectCommon.h
//------------------------------------------------------------------------------
// Copyright (c) 2021 John Hughes
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
#ifndef NETOBJECTCOMMON_H
#define NETOBJECTCOMMON_H

//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "ae/aether.h"
#include "ae/aetherEXT.h"

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
const ae::Tag TAG_EXAMPLE = "example";
const AetherMsgId kObjectInfoMsg = 1;

//------------------------------------------------------------------------------
// Game class
//------------------------------------------------------------------------------
class Game
{
public:
  void Initialize( const char* windowTitle )
  {
    window.Initialize( 800, 600, false, true );
    window.SetTitle( windowTitle );
    render.Initialize( &window );
    input.Initialize( &window );
    timeStep.SetTimeStep( 1.0f / 10.0f );
    debugLines.Initialize( 32 );
  }

  void Terminate()
  {
    debugLines.Terminate();
    //input.Terminate();
    render.Terminate();
    window.Terminate();
  }

  void Render( const ae::Matrix4& worldToNdc )
  {
    render.Activate();
    render.Clear( ae::Color::PicoBlack() );
    
    debugLines.Render( worldToNdc );
    
    render.Present();
    timeStep.Wait();
  }
  
  ae::Window window;
  ae::GraphicsDevice render;
  ae::Input input;
  ae::TimeStep timeStep;
  ae::DebugLines debugLines;
};

//------------------------------------------------------------------------------
// GameObject class
//------------------------------------------------------------------------------
class GameObject
{
public:
  GameObject( ae::Color color )
  {
    netObject = nullptr;
    alive = true;
    playerId = AetherUuid::Zero();
    m_pos = ae::Vec3( aeMath::Random( -10.0f, 10.0f ), aeMath::Random( -10.0f, 10.0f ), 0.0f );
    m_radius = aeMath::Random( 0.5f, 2.0f );
    m_color = color;
  }

  void Update( Game* game )
  {
    // Client - read net data
    if ( !netObject->IsAuthority() )
    {
      ae::BinaryStream rStream = ae::BinaryStream::Reader( netObject->GetSyncData(), netObject->SyncDataLength() );
      Serialize( &rStream );
    }

    // Server - write net data
    if ( netObject->IsAuthority() )
    {
      ae::BinaryStream wStream = ae::BinaryStream::Writer();
      Serialize( &wStream );
      netObject->SetSyncData( wStream.GetData(), wStream.GetOffset() );
    }

    // Draw
    uint32_t points = ( 2.0f * ae::PI * m_radius ) / 0.25f + 0.5f;
    game->debugLines.AddCircle( m_pos, ae::Vec3( 0, 0, 1 ), m_radius, m_color, points );
  }

  void Serialize( ae::BinaryStream* stream )
  {
    stream->SerializeFloat( m_pos.x );
    stream->SerializeFloat( m_pos.y );
    stream->SerializeFloat( m_pos.z );
    stream->SerializeFloat( m_radius );
    stream->SerializeFloat( m_color.r );
    stream->SerializeFloat( m_color.g );
    stream->SerializeFloat( m_color.b );
  }

  ae::NetObject* netObject;
  bool alive;
  AetherUuid playerId;

private:
  ae::Vec3 m_pos;
  float m_radius;
  ae::Color m_color;
};

#endif