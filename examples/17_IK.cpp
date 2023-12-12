//------------------------------------------------------------------------------
// 16_IK.cpp
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
// Headers
//------------------------------------------------------------------------------
#include "aether.h"
#include "ae/loaders.h"
#include "ae/aeImGui.h"
#include "ImGuizmo.h"

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
const ae::Tag TAG_ALL = "all";

struct Vertex
{
	ae::Vec4 pos;
	ae::Vec4 normal;
	ae::Vec4 color;
	ae::Vec2 uv;
};

//------------------------------------------------------------------------------
// Shaders
//------------------------------------------------------------------------------
const char* kVertShader = R"(
	AE_UNIFORM mat4 u_worldToProj;
	AE_UNIFORM mat4 u_normalToWorld;
	AE_UNIFORM vec4 u_color;
	AE_IN_HIGHP vec4 a_position;
	AE_IN_HIGHP vec4 a_normal;
	AE_IN_HIGHP vec4 a_color;
	AE_IN_HIGHP vec2 a_uv;
	AE_OUT_HIGHP vec3 v_normal;
	AE_OUT_HIGHP vec4 v_color;
	AE_OUT_HIGHP vec2 v_uv;
	void main()
	{
		v_normal = (u_normalToWorld * a_normal).xyz;
		v_color = a_color * u_color;
		v_uv = a_uv;
		gl_Position = u_worldToProj * a_position;
	})";

const char* kFragShader = R"(
	AE_UNIFORM sampler2D u_tex;
	AE_UNIFORM vec3 u_lightDir;
	AE_UNIFORM vec3 u_lightColor;
	AE_UNIFORM vec3 u_ambColor;
	AE_IN_HIGHP vec3 v_normal;
	AE_IN_HIGHP vec4 v_color;
	AE_IN_HIGHP vec2 v_uv;
	void main()
	{
		vec4 diff = AE_TEXTURE2D( u_tex, v_uv );
		float lightAmt = max(0.0, dot(normalize(v_normal), -u_lightDir));
		vec3 light = u_ambColor + lightAmt * u_lightColor;
		AE_COLOR.rgb = diff.rgb * v_color.rgb * light;
		AE_COLOR.a = diff.a * v_color.a;
		
	})";

ae::Vec2 GetNearestPointOnEllipse( ae::Vec2 halfSize, ae::Vec2 center, ae::Vec2 p )
{
	// https://stackoverflow.com/a/46007540/2423134
	// https://blog.chatfield.io/simple-method-for-distance-to-ellipse/
	// https://github.com/0xfaded/ellipse_demo/issues/1
	const float px = ae::Abs( p[ 0 ] );
	const float py = ae::Abs( p[ 1 ] );
	const float a = ae::Abs( halfSize.x );
	const float b = ae::Abs( halfSize.y );

	float tx = 0.707f;
	float ty = 0.707f;
	// Only 3 iterations should be needed for high quality results
	for( uint32_t i = 0; i < 3; i++ )
	{
		const float x = a * tx;
		const float y = b * ty;
		const float ex = ( a * a - b * b ) * ( tx * tx * tx ) / a;
		const float ey = ( b * b - a * a ) * ( ty * ty * ty ) / b;
		const float rx = x - ex;
		const float ry = y - ey;
		const float qx = px - ex;
		const float qy = py - ey;
		const float r = hypotf( ry, rx );
		const float q = hypotf( qy, qx );
		tx = ae::Min( 1.0f, ae::Max( 0.0f, ( qx * r / q + ex ) / a ) );
		ty = ae::Min( 1.0f, ae::Max( 0.0f, ( qy * r / q + ey ) / b ) );
		const float t = hypotf( ty, tx );
		tx /= t;
		ty /= t;
	}

	return ae::Vec2( copysignf( a * tx, p[ 0 ] ), copysignf( b * ty, p[ 1 ] ) );
}

//------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------
int main()
{
	AE_INFO( "Initialize" );

	ae::Window window;
	ae::GraphicsDevice render;
	ae::Input input;
	ae::TimeStep timeStep;
	ae::Shader shader;
	ae::FileSystem fileSystem;
	ae::DebugCamera camera = ae::Axis::Z;
	ae::DebugLines debugLines = TAG_ALL;
	ae::DebugLines gridLines = TAG_ALL;
	aeImGui ui;

	window.Initialize( 800, 600, false, true );
	window.SetTitle( "17_IK" );
	// window.SetAlwaysOnTop( ae::IsDebuggerAttached() );
	render.Initialize( &window );
	input.Initialize( &window );
	timeStep.SetTimeStep( 1.0f / 60.0f );
	fileSystem.Initialize( "data", "ae", "ik" );
	camera.Reset( ae::Vec3( 0.0f, 0.0f, 1.0f ), ae::Vec3( 0.0f, 3.5f, 0.4f ) );
	camera.SetDistanceLimits( 1.0f, 25.0f );
	debugLines.Initialize( 4096 );
	gridLines.Initialize( 4096 );
	gridLines.SetXRayEnabled( false );
	ui.Initialize();

	shader.Initialize( kVertShader, kFragShader, nullptr, 0 );
	shader.SetDepthTest( true );
	shader.SetDepthWrite( true );
	shader.SetBlending( true );
	shader.SetCulling( ae::Culling::CounterclockwiseFront );

	ae::Texture2D texture;
	{
		ae::TargaFile targaFile = TAG_ALL;
		uint32_t fileSize = fileSystem.GetSize( ae::FileSystem::Root::Data, "character.tga" );
		AE_ASSERT( fileSize );
		ae::Scratch< uint8_t > fileData( fileSize );
		fileSystem.Read( ae::FileSystem::Root::Data, "character.tga", fileData.Data(), fileData.Length() );
		targaFile.Load( fileData.Data(), fileData.Length() );
		texture.Initialize( targaFile.textureParams );
	}
	
	ae::Skin skin = TAG_ALL;
	ae::VertexBuffer vertexData;
	Vertex* vertices = nullptr;
	{
		const char* fileName = "character.fbx";
		uint32_t fileSize = fileSystem.GetSize( ae::FileSystem::Root::Data, fileName );
		AE_ASSERT_MSG( fileSize, "Could not load '#'", fileName );
		ae::Scratch< uint8_t > fileData( fileSize );
		if ( !fileSystem.Read( ae::FileSystem::Root::Data, fileName, fileData.Data(), fileData.Length() ) )
		{
			AE_ERR( "Error reading fbx file: '#'", fileName );
			return -1;
		}
		
		ae::FbxLoader fbxLoader = TAG_ALL;
		if ( !fbxLoader.Initialize( fileData.Data(), fileData.Length() ) )
		{
			AE_ERR( "Error parsing fbx file: '#'", fileName );
			return -1;
		}
		
		ae::FbxLoaderParams params;
		params.descriptor.vertexSize = sizeof(Vertex);
		params.descriptor.indexSize = 4;
		params.descriptor.posOffset = offsetof( Vertex, pos );
		params.descriptor.normalOffset = offsetof( Vertex, normal );
		params.descriptor.colorOffset = offsetof( Vertex, color );
		params.descriptor.uvOffset = offsetof( Vertex, uv );
		params.vertexData = &vertexData;
		params.skin = &skin;
		params.maxVerts = fbxLoader.GetMeshVertexCount( 0u );
		vertices = ae::NewArray< Vertex >( TAG_ALL, params.maxVerts );
		params.vertexOut = vertices;
		if ( !fbxLoader.Load( fbxLoader.GetMeshName( 0 ), params ) )
		{
			AE_ERR( "Error loading fbx file data: '#'", fileName );
			return -1;
		}
	}
	
	// const char* anchorBoneName = "QuickRigCharacter_Hips";
	// const char* headBoneName = "QuickRigCharacter_Head";
	// const char* rightHandBoneName = "QuickRigCharacter_RightHand";
	// const char* leftHandBoneName = "QuickRigCharacter_LeftHand";
	// const char* leftFootBoneName = "QuickRigCharacter_LeftFoot";
	// const char* rightFootBoneName = "QuickRigCharacter_RightFoot";

	const char* rightHandBoneName = "QuickRigCharacter_RightHand";
	const char* anchorBoneName = "QuickRigCharacter_RightShoulder";

	ae::Skeleton currentPose = TAG_ALL;
	ae::Matrix4 targetTransform;
	ae::Matrix4 testJointHandle;
	auto SetDefault = [&]()
	{
		testJointHandle = ae::Matrix4::Translation( ae::Vec3( 0.0f, 0.0f, 2.0f ) ) * ae::Matrix4::Scaling( 0.1f );
		targetTransform = skin.GetBindPose().GetBoneByName( rightHandBoneName )->transform;
		currentPose.Initialize( &skin.GetBindPose() );
	};
	SetDefault();
	ImGuizmo::OPERATION gizmoOperation = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE gizmoMode = ImGuizmo::WORLD;
	bool drawMesh = true;
	bool drawSkeleton = true;
	bool autoIK = true;
	bool moveTestJoint = false;
	int selection = 1;
	auto GetSelectedTransform = [&]() -> ae::Matrix4&
	{
		if ( moveTestJoint )
		{
			return testJointHandle;
		}
		return targetTransform;
	};
	float angleLimit[ 4 ] = { ae::QuarterPi, ae::QuarterPi, ae::QuarterPi, ae::QuarterPi };
	
	AE_INFO( "Run" );
	while ( !input.quit )
	{
		const float dt = ae::Max( timeStep.GetTimeStep(), timeStep.GetDt() );
		input.Pump();

		ImGuiIO& io = ImGui::GetIO();
		ui.NewFrame( &render, &input, dt );
		ImGuizmo::SetOrthographic( false );
		ImGuizmo::SetRect( 0, 0, io.DisplaySize.x, io.DisplaySize.y );
		ImGuizmo::AllowAxisFlip( false );
		ImGuizmo::BeginFrame();

		bool shouldStep = false;
		ImGui::SetNextWindowPos( ImVec2( 0, 0 ), ImGuiCond_FirstUseEver );
		ImGui::SetNextWindowSize( ImVec2( 200, 300 ), ImGuiCond_FirstUseEver );
		if ( ImGui::Begin( "Options" ) )
		{
			ImGui::Checkbox( "Draw Mesh", &drawMesh );
			ImGui::Checkbox( "Auto IK", &autoIK );
			ImGui::SameLine();
			ImGui::BeginDisabled( autoIK );
			if ( ImGui::Button( "Step" ) )
			{
				shouldStep = true;
			}
			ImGui::EndDisabled();
			ImGui::Checkbox( "Draw Skeleton", &drawSkeleton );

			ImGui::Separator();

			ImGui::RadioButton( "Translate", (int*)&gizmoOperation, ImGuizmo::TRANSLATE );
			ImGui::SameLine();
			ImGui::RadioButton( "Rotate", (int*)&gizmoOperation, ImGuizmo::ROTATE );

			ImGui::RadioButton( "World", (int*)&gizmoMode, ImGuizmo::WORLD );
			ImGui::SameLine();
			ImGui::RadioButton( "Local", (int*)&gizmoMode, ImGuizmo::LOCAL );

			ImGui::Separator();

			if ( ImGui::Button( "Reset" ) )
			{
				SetDefault();
			}
			ImGui::SameLine();
			if ( ImGui::Button( "Focus" ) )
			{
				camera.Refocus( GetSelectedTransform().GetTranslation() );
			}

			ImGui::Separator();

			ImGui::Checkbox( "Move Test Joint", &moveTestJoint );
			ImGui::SliderFloat( "T0", &angleLimit[ 0 ], 0.0f, ae::HalfPi );
			ImGui::SliderFloat( "T1", &angleLimit[ 1 ], 0.0f, ae::HalfPi );
			ImGui::SliderFloat( "T2", &angleLimit[ 2 ], 0.0f, ae::HalfPi );
			ImGui::SliderFloat( "T3", &angleLimit[ 3 ], 0.0f, ae::HalfPi );
		}
		if ( input.GetPress( ae::Key::V ) )
		{
			drawMesh = !drawMesh;
		}
		if ( input.GetPress( ae::Key::S ) )
		{
			drawSkeleton = !drawSkeleton;
		}
		if ( input.GetPress( ae::Key::W ) )
		{
			if ( gizmoOperation != ImGuizmo::TRANSLATE ) { gizmoOperation = ImGuizmo::TRANSLATE; }
			else { gizmoMode = ( gizmoMode == ImGuizmo::WORLD ) ? ImGuizmo::LOCAL : ImGuizmo::WORLD; }
		}
		if ( input.GetPress( ae::Key::E ) )
		{
			if ( gizmoOperation != ImGuizmo::ROTATE ) { gizmoOperation = ImGuizmo::ROTATE; }
			else { gizmoMode = ( gizmoMode == ImGuizmo::WORLD ) ? ImGuizmo::LOCAL : ImGuizmo::WORLD; }
		}
		if ( input.GetPress( ae::Key::R ) )
		{
			SetDefault();
		}
		if ( input.GetPress( ae::Key::F ) )
		{
			camera.Refocus( GetSelectedTransform().GetTranslation() );
		}
		if ( input.GetPress( ae::Key::I ) )
		{
			autoIK = !autoIK;
		}
		if ( input.Get( ae::Key::Num1 ) ) { selection = 1; }
		if ( input.Get( ae::Key::Num2 ) ) { selection = 2; }
		if ( input.Get( ae::Key::Num3 ) ) { selection = 3; }
		if ( input.Get( ae::Key::Num4 ) ) { selection = 4; }
		if ( !autoIK && input.GetPress( ae::Key::Space ) )
		{
			shouldStep = true;
		}
		ImGui::End();
		
		camera.SetInputEnabled( !ImGui::GetIO().WantCaptureMouse && !ImGuizmo::IsUsing() );
		camera.Update( &input, dt );
		
		if ( autoIK || shouldStep )
		{
			ae::IK ik = TAG_ALL;
			const ae::Bone* extentBone = currentPose.GetBoneByName( rightHandBoneName );
			for ( auto b = extentBone; b; b = b->parent )
			{
				ik.chain.Insert( 0, b->index );
				if ( b->name == anchorBoneName )
				{
					break;
				}
			}
			// ae::IKChain* chain = &ik.chains.Append();
			// chain->bones.Append( { extentBone->transform, extentBone->parent->transform.GetTranslation() } );
			ik.targetTransform = targetTransform;
			ik.pose.Initialize( &currentPose );
			for ( uint32_t idx : ik.chain )
			{
				ae::IKJoint joint;
				const ae::Bone* bone = ik.pose.GetBoneByIndex( idx );
				const bool right = memcmp( "QuickRigCharacter_Right", bone->name.c_str(), strlen( "QuickRigCharacter_Right" ) ) == 0;
				joint.primaryAxis = right ? ae::Vec3( -1, 0, 0 ) : ae::Vec3( 1, 0, 0 );
				ik.joints.Append( joint );
			}
			ik.Run( autoIK ? 10 : 1, &currentPose );

			// Debug
			if ( drawSkeleton )
			{
				for ( uint32_t idx : ik.chain )
				{
					debugLines.AddOBB( currentPose.GetBoneByIndex( idx )->transform * ae::Matrix4::Scaling( 0.1f ), ae::Color::Magenta() );
				}
			}
		}
		
		// Update mesh
		skin.ApplyPoseToMesh( &currentPose, vertices->pos.data, vertices->normal.data, sizeof(Vertex), sizeof(Vertex), true, true, vertexData.GetMaxVertexCount() );
		vertexData.UploadVertices( 0, vertices, vertexData.GetMaxVertexCount() );
		
		// Debug
		if ( drawSkeleton )
		{
			for ( uint32_t i = 0; i < currentPose.GetBoneCount(); i++ )
			{
				const ae::Matrix4& t = currentPose.GetBoneByIndex( i )->transform;
				ae::Vec3 p = t.GetTranslation();
				ae::Vec3 xAxis = t.GetAxis( 0 );
				ae::Vec3 yAxis = t.GetAxis( 1 );
				ae::Vec3 zAxis = t.GetAxis( 2 );
				debugLines.AddLine( p, p + xAxis * 0.2f, ae::Color::Red() );
				debugLines.AddLine( p, p + yAxis * 0.2f, ae::Color::Green() );
				debugLines.AddLine( p, p + zAxis * 0.2f, ae::Color::Blue() );
			}

			for ( uint32_t i = 0; i < currentPose.GetBoneCount(); i++ )
			{
				const ae::Bone* bone = currentPose.GetBoneByIndex( i );
				const ae::Bone* parent = bone->parent;
				if ( parent )
				{
					debugLines.AddLine( parent->transform.GetTranslation(), bone->transform.GetTranslation(), ae::Color::PicoBlue() );
					debugLines.AddOBB( bone->transform * ae::Matrix4::Scaling( 0.05f ), ae::Color::PicoBlue() );
				}
			}
		}

		const ae::Vec3 testJoint( 0.0f, 0.0f, -2.0f );
		const ae::Vec3 testJointNext( 0.0f, 0.0f, 2.0f );
		const ae::Vec3 jointSegment = testJointNext - testJoint;
		const float jointLen = jointSegment.Length();
		const ae::Vec3 currJointDir = ( testJointHandle.GetTranslation() - testJoint );
		ae::Vec3 jointProj( 0.0f );
		ae::Plane( ae::Vec3( 0.0f ), ae::Vec3( 0, 0, 1 ) ).IntersectLine( testJointHandle.GetTranslation(), currJointDir, &jointProj );
		const float q[ 4 ] = {
			ae::Atan( ae::Max( angleLimit[ 0 ], 0.0001f ) ),
			ae::Atan( ae::Max( angleLimit[ 1 ], 0.0001f ) ),
			ae::Atan( -ae::Max( angleLimit[ 2 ], 0.0001f ) ),
			ae::Atan( -ae::Max( angleLimit[ 3 ], 0.0001f ) )
		};
		ae::Color quadrantColor[ 4 ] = { ae::Color::Green(), ae::Color::Green(), ae::Color::Green(), ae::Color::Green() };
		const ae::Vec2 quadrantEllipse = [q, jointProj, &quadrantColor](){
			if( jointProj.x > 0.0f && jointProj.y > 0.0f ) { quadrantColor[ 0 ] = ae::Color::Red(); return ae::Vec2( q[ 0 ], q[ 1 ] ); } // +x +y
			if( jointProj.x < 0.0f && jointProj.y > 0.0f ) { quadrantColor[ 1 ] = ae::Color::Red(); return ae::Vec2( q[ 2 ], q[ 1 ] ); } // -x +y
			if( jointProj.x < 0.0f && jointProj.y < 0.0f ) { quadrantColor[ 2 ] = ae::Color::Red(); return ae::Vec2( q[ 2 ], q[ 3 ] ); } // -x -y
			quadrantColor[ 3 ] = ae::Color::Red();
			return ae::Vec2( q[ 0 ], q[ 3 ] ); // +x -y
		}();
		const ae::Vec2 edge = GetNearestPointOnEllipse( quadrantEllipse, ae::Vec2( 0.0f ), jointProj.GetXY() );
		ae::Vec3 jointProjClipped = jointProj;
		if( jointProjClipped.GetXY().LengthSquared() > edge.LengthSquared() )
		{
			jointProjClipped.SetXY( edge );
		}
		const ae::Vec3 jointEnd1 = testJoint + ( jointProjClipped - testJoint ).NormalizeCopy() * jointLen;

		// Joint limits
		debugLines.AddOBB( testJointHandle, ae::Color::Magenta() );
		debugLines.AddLine( testJoint, jointEnd1, ae::Color::PicoPink() );
		debugLines.AddCircle( jointProj, ae::Vec3( 0, 0, 1 ), 0.1f, ae::Color::Magenta(), 16 );
		debugLines.AddCircle( jointProjClipped, ae::Vec3( 0, 0, 1 ), 0.1f, ae::Color::Magenta(), 16 );
		debugLines.AddLine( testJoint, ae::Vec3( q[ 0 ], 0.0f, 0.0f ), ae::Color::Green() );
		debugLines.AddLine( testJoint, ae::Vec3( 0.0f, q[ 1 ], 0.0f ), ae::Color::Green() );
		debugLines.AddLine( testJoint, ae::Vec3( q[ 2 ], 0.0f, 0.0f ), ae::Color::Green() );
		debugLines.AddLine( testJoint, ae::Vec3( 0.0f, q[ 3 ], 0.0f ), ae::Color::Green() );
		for ( uint32_t i = 0; i < 16; i++ )
		{
			const ae::Vec3 q0( q[ 0 ], q[ 1 ], 0 ); // +x +y
			const ae::Vec3 q1( q[ 2 ], q[ 1 ], 0 ); // -x +y
			const ae::Vec3 q2( q[ 2 ], q[ 3 ], 0 ); // -x -y
			const ae::Vec3 q3( q[ 0 ], q[ 3 ], 0 ); // +x -y
			const float step = ( ae::HalfPi / 16 );
			const float angle = i * step;
			const ae::Vec3 p0( ae::Cos( angle ), ae::Sin( angle ), 0 );
			const ae::Vec3 p1( ae::Cos( angle + step ), ae::Sin( angle + step ), 0 );
			debugLines.AddLine( p0 * q0, p1 * q0, quadrantColor[ 0 ] );
			debugLines.AddLine( p0 * q1, p1 * q1, quadrantColor[ 1 ] );
			debugLines.AddLine( p0 * q2, p1 * q2, quadrantColor[ 2 ] );
			debugLines.AddLine( p0 * q3, p1 * q3, quadrantColor[ 3 ] );
		}

		// Add grid
		gridLines.AddLine( ae::Vec3( -2, 0, 0 ), ae::Vec3( 2, 0, 0 ), ae::Color::Red() );
		gridLines.AddLine( ae::Vec3( 0, -2, 0 ), ae::Vec3( 0, 2, 0 ), ae::Color::Green() );
		for ( float i = -2; i <= 2.00001f; i += 0.2f )
		{
			if ( ae::Abs( i ) < 0.0001f ) { continue; }
			gridLines.AddLine( ae::Vec3( i, -2, 0 ), ae::Vec3( i, 2, 0 ), ae::Color::PicoLightGray() );
			gridLines.AddLine( ae::Vec3( -2, i, 0 ), ae::Vec3( 2, i, 0 ), ae::Color::PicoLightGray() );
		}
		
		// Start frame
		ae::Matrix4 worldToView = ae::Matrix4::WorldToView( camera.GetPosition(), camera.GetForward(), camera.GetLocalUp() );
		ae::Matrix4 viewToProj = ae::Matrix4::ViewToProjection( 0.9f, render.GetAspectRatio(), 0.25f, 50.0f );
		ae::Matrix4 worldToProj = viewToProj * worldToView;
		
		ImGuizmo::Manipulate(
			worldToView.data,
			viewToProj.data,
			gizmoOperation,
			gizmoMode,
			GetSelectedTransform().data
		);
		
		render.Activate();
		render.Clear( window.GetFocused() ? ae::Color::AetherBlack() : ae::Color::PicoBlack() );
		
		// Render mesh
		if ( drawMesh )
		{
			ae::Matrix4 modelToWorld = ae::Matrix4::Identity();
			ae::UniformList uniformList;
			uniformList.Set( "u_worldToProj", worldToProj * modelToWorld );
			uniformList.Set( "u_normalToWorld", modelToWorld.GetNormalMatrix() );
			uniformList.Set( "u_lightDir", ae::Vec3( 0.0f, -1.0f, 0.0f ).NormalizeCopy() );
			uniformList.Set( "u_lightColor", ae::Color::PicoPeach().GetLinearRGB() );
			uniformList.Set( "u_ambColor", ae::Vec3( 0.8f ) );
			uniformList.Set( "u_color", ae::Color::White().GetLinearRGBA() );
			uniformList.Set( "u_tex", &texture );
			vertexData.Bind( &shader, uniformList );
			vertexData.Draw();
		}
		
		// Frame end
		debugLines.Render( worldToProj );
		gridLines.Render( worldToProj );
		ui.Render();
		render.Present();
		timeStep.Tick();
	}

	AE_INFO( "Terminate" );
	ae::Delete( vertices );
	vertices = nullptr;
	input.Terminate();
	render.Terminate();
	window.Terminate();

	return 0;
}
