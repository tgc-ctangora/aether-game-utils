//------------------------------------------------------------------------------
// 04_ReplicationServer.cpp
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
// Headers
//------------------------------------------------------------------------------
#include "ae/aetherEXT.h"
#include "04_ReplicationCommon.h"

void Game::Initialize()
{
  window.Initialize( 800, 600, false, true );
  window.SetTitle( "Replication Server" );
  render.Initialize( &window );
  input.Initialize( &window );
  timeStep.SetTimeStep( 1.0f / 10.0f );
  const char* vertexStr = R"(
    AE_UNIFORM_HIGHP mat4 u_worldToNdc;
    AE_IN_HIGHP vec4 a_position;
    AE_IN_HIGHP vec4 a_color;
    AE_OUT_HIGHP vec4 v_color;
    void main()
    {
      v_color = a_color;
      gl_Position = u_worldToNdc * a_position;
    })";
  const char* fragStr = R"(
    AE_IN_HIGHP vec4 v_color;
    void main()
    {
      AE_COLOR = v_color;
    })";
  m_shader.Initialize( vertexStr, fragStr, nullptr, 0 );
  m_spriteData.Initialize( sizeof(Vertex), sizeof(Index), 4, 6, ae::VertexData::Primitive::Triangle, ae::VertexData::Usage::Dynamic, ae::VertexData::Usage::Dynamic );
  m_spriteData.AddAttribute( "a_position", 4, ae::VertexData::Type::Float, offsetof(Vertex, position) );
  m_spriteData.AddAttribute( "a_color", 4, ae::VertexData::Type::Float, offsetof(Vertex, color) );
}

void Game::Terminate()
{
  //input.Terminate();
  render.Terminate();
  window.Terminate();
}

void Game::Render( const ae::Matrix4& worldToNdc )
{
  render.Activate();
  render.Clear( aeColor::PicoBlack() );
  
  Vertex vertex[] = {
    { m_sprites[ 0 ].transform * ae::Vec4( -0.5f, -0.5f, 0.0f, 1.0f ), m_sprites[ 0 ].color },
    { m_sprites[ 0 ].transform * ae::Vec4( 0.5f, -0.5f, 0.0f, 1.0f ), m_sprites[ 0 ].color },
    { m_sprites[ 0 ].transform * ae::Vec4( 0.5f, 0.5f, 0.0f, 1.0f ), m_sprites[ 0 ].color },
    { m_sprites[ 0 ].transform * ae::Vec4( -0.5f, 0.5f, 0.0f, 1.0f ), m_sprites[ 0 ].color }
  };
  Index indices[] = {
    3, 0, 1,
    3, 1, 2
  };
  m_spriteData.SetVertices( vertex, countof(vertex) );
  m_spriteData.SetIndices( indices, countof(indices) );
  
  ae::UniformList uniforms;
  uniforms.Set( "u_worldToNdc", worldToNdc );
  m_spriteData.Render( &m_shader, uniforms );
  
  render.Present();
  timeStep.Wait();
}

void Game::SetSprite( uint32_t index, const ae::Matrix4& transform, ae::Color color )
{
  if ( index < countof(m_sprites) )
  {
    m_sprites[ index ].transform = transform;
    m_sprites[ index ].color = color;
  }
}

//------------------------------------------------------------------------------
// ClientInfo class
//------------------------------------------------------------------------------
class ClientInfo
{
public:
  AetherUuid uuid;
  aeNetReplicaServer* replicaServer = nullptr;
};

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------
int main()
{
  AE_LOG( "Initialize" );

  Game game;
  game.Initialize();
  game.SetSprite( 0, ae::Matrix4::Identity(), ae::Color::Green() );

  // Server modules
  AetherServer* server = AetherServer_New( 3500, 0, 1 );
  aeNetReplicaDB replicaDB;
  ae::Map< AetherUuid, aeNetReplicaServer* > replicaServers = TAG_EXAMPLE;

  // Game data
  ae::Array< Green > greens = TAG_EXAMPLE;
  
  // Net update
  while ( !game.input.quit )
  {
    game.input.Pump();
    AetherServer_Update( server );
    
    ServerReceiveInfo receiveInfo;
    while ( AetherServer_Receive( server, &receiveInfo ) )
    {
      switch ( receiveInfo.msgId )
      {
        case kSysMsgPlayerConnect:
        {
          AE_LOG( "Player # connected", receiveInfo.player->uuid );
          replicaServers.Set( receiveInfo.player->uuid, replicaDB.CreateServer() );
          break;
        }
        case kSysMsgPlayerDisconnect:
        {
          AE_LOG( "Player # disconnected", receiveInfo.player->uuid );

          aeNetReplicaServer* replicaServer = nullptr;
          if ( replicaServers.TryGet( receiveInfo.player->uuid, &replicaServer ) )
          {
            replicaServers.Remove( receiveInfo.player->uuid );
            replicaDB.DestroyServer( replicaServer );
          }

          break;
        }
        default:
        {
          break;
        }
      }
    }

    // Game Update
    while ( greens.Length() < 3 )
    {
      Green* green = &greens.Append( Green() );
      green->pos = aeFloat3( aeMath::Random( -10.0f, 10.0f ), aeMath::Random( -10.0f, 10.0f ), 0.0f );
      green->size = aeFloat3( aeMath::Random( 0.5f, 2.0f ), aeMath::Random( 0.5f, 2.0f ), 1.0f );
      green->rotation = aeMath::Random( 0.0f, aeMath::TWO_PI );
      green->life = 5.0f + aeMath::Random( 0.7f, 1.3f );
      green->netData = replicaDB.CreateNetData();
      green->netData->SetInitData( &kReplicaType_Green, sizeof( kReplicaType_Green ) );
    }
    for ( uint32_t i = 0; i < greens.Length(); i++ )
    {
      greens[ i ].Update( &game );
    }

    // Destroy dead objects
    auto findDeadFn = []( const Green& object ){ return object.life <= 0.0f; };
    for ( int32_t index = greens.FindFn( findDeadFn ); index >= 0; index = greens.FindFn( findDeadFn ) )
    {
      replicaDB.DestroyNetData( greens[ index ].netData );
      greens.Remove( index );
    }
    
    // Send replication data
    replicaDB.UpdateSendData();
    for ( int32_t i = 0; i < server->playerCount; i++ )
    {
      AetherPlayer* player = server->allPlayers[ i ];
      aeNetReplicaServer* replicaServer = nullptr;
      if ( replicaServers.TryGet( player->uuid, &replicaServer ) )
      {
        AetherServer_QueueSendToPlayer( server, player, kReplicaInfoMsg, true, replicaServer->GetSendData(), replicaServer->GetSendLength() );
      }
    }
    AetherServer_SendAll( server );

    game.Render( ae::Matrix4::Scaling( aeFloat3( 1.0f / ( 10.0f * game.render.GetAspectRatio() ), 1.0f / 10.0f, 1.0f ) ) );
  }

  AE_LOG( "Terminate" );

  AetherServer_Delete( server );
  server = nullptr;
  
  game.Terminate();

  return 0;
}
