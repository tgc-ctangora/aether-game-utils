//------------------------------------------------------------------------------
// Entity.h
//------------------------------------------------------------------------------
// Copyright (c) 2021 John Hughes
// Created by John Hughes on 3/13/21.
//------------------------------------------------------------------------------
#ifndef ENTITY_H
#define ENTITY_H

//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "aether.h"
namespace ae {

//------------------------------------------------------------------------------
// Entity
//------------------------------------------------------------------------------
typedef uint32_t Entity;
const Entity kInvalidEntity = 0;
class EditorLevel;

//------------------------------------------------------------------------------
// Component
//------------------------------------------------------------------------------
class Component : public ae::Inheritor< ae::Object, Component >
{
public:
	Entity GetEntity() const { return m_entity; }
	const char* GetEntityName() const;
	
	Component& GetComponent( const char* typeName );
	template < typename T > T& GetComponent();
	const Component& GetComponent( const char* typeName ) const;
	template < typename T > const T& GetComponent() const;
	
	Component* TryGetComponent( const char* typeName );
	template < typename T > T* TryGetComponent();
	const Component* TryGetComponent( const char* typeName ) const;
	template < typename T > const T* TryGetComponent() const;
	
private:
	friend class Registry;
	class Registry* m_reg = nullptr;
	Entity m_entity = kInvalidEntity;
};

typedef std::function< void( const class EditorObject& levelObject, Entity entity, class Registry* registry ) > CreateObjectFn;

//------------------------------------------------------------------------------
// Registry
//------------------------------------------------------------------------------
class Registry
{
public:
	Registry( const ae::Tag& tag );
	void SetOnCreateFn( std::function< void(Component*) > fn );
	
	// Creation
	Entity CreateEntity( const char* name = "" );
	Entity CreateEntity( Entity entity, const char* name = "" );
	Component* AddComponent( Entity entity, const char* typeName );
	template < typename T > T* AddComponent( Entity entity );
	bool Load( const ae::EditorLevel* level, CreateObjectFn fn = nullptr );
	
	// Get reference
	Component& GetComponent( Entity entity, const char* typeName );
	Component& GetComponent( const char* name, const char* typeName );
	template < typename T > T& GetComponent( Entity entity );
	template < typename T > T& GetComponent( const char* name );
	
	// Try get pointer
	Component* TryGetComponent( Entity entity, const char* typeName );
	Component* TryGetComponent( const char* name, const char* typeName );
	template < typename T > T* TryGetComponent( Entity entity );
	template < typename T > T* TryGetComponent( const char* name );
	
	// Templated iteration over component type
	template < typename T > uint32_t GetComponentCount() const;
	template < typename T > Entity GetEntityByIndex( uint32_t index );
	template < typename T > T& GetComponentByIndex( uint32_t index );
	
	// Entity names
	Entity GetEntityByName( const char* name ) const;
	const char* GetNameByEntity( Entity entity ) const;
	void SetEntityName( Entity entity, const char* name );
	
	// Iterate over all entities/components
	uint32_t GetTypeCount() const;
	const ae::Type* GetTypeByIndex( uint32_t typeIndex ) const;
	int32_t GetTypeIndexByType( const ae::Type* type ) const;
	uint32_t GetComponentCountByIndex( uint32_t typeIndex ) const;
	const Component& GetComponentByIndex( uint32_t typeIndex, uint32_t componentIndex ) const;
	Component& GetComponentByIndex( uint32_t typeIndex, uint32_t componentIndex );
	template < typename T, typename Fn > uint32_t CallFn( Fn fn );
	
	// Removal
	void Clear();
	
private:
	const ae::Tag m_tag;
	Entity m_lastEntity = kInvalidEntity;
	ae::Map< ae::Str16, Entity > m_entityNames;
	ae::Map< ae::TypeId, ae::Map< Entity, Component* > > m_components;
	std::function< void(Component*) > m_onCreate;
};

//------------------------------------------------------------------------------
// Component member functions
//------------------------------------------------------------------------------
template < typename T >
T& Component::GetComponent()
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	AE_ASSERT( m_reg );
	T* t = m_reg->TryGetComponent< T >( m_entity );
	if ( !t )
	{
		AE_FAIL_MSG( "Component '#' has no sibling '#'", ae::GetTypeFromObject( this )->GetName(), ae::GetType< T >()->GetName() );
	}
	return *t;
}

template < typename T >
T* Component::TryGetComponent()
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	AE_ASSERT( m_reg );
	return m_reg->TryGetComponent< T >( m_entity );
}

template < typename T >
const T& Component::GetComponent() const
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	AE_ASSERT( m_reg );
	T* t = m_reg->TryGetComponent< T >( m_entity );
	if ( !t )
	{
		AE_FAIL_MSG( "Component '#' has no sibling '#'", ae::GetTypeFromObject( this )->GetName(), ae::GetType< T >()->GetName() );
	}
	return *t;
}

template < typename T >
const T* Component::TryGetComponent() const
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	AE_ASSERT( m_reg );
	return m_reg->TryGetComponent< T >( m_entity );
}

//------------------------------------------------------------------------------
// Registry member functions
//------------------------------------------------------------------------------
template < typename T >
T* Registry::AddComponent( Entity entity )
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	const ae::Type* type = ae::GetType< T >();
	AE_ASSERT( type );
	
	ae::Object* object = (ae::Object*)ae::Allocate( m_tag, type->GetSize(), type->GetAlignment() );
	type->New( object );
	
	Component* component = ae::Cast< T >( object );
	AE_ASSERT( component );
	component->m_entity = entity;
	component->m_reg = this;
	
	ae::Map< Entity, Component* >* components = m_components.TryGet( type->GetId() );
	if ( !components )
	{
		components = &m_components.Set( type->GetId(), m_tag );
	}
	components->Set( entity, component );
	
	if ( m_onCreate )
	{
		m_onCreate( component );
	}
	return (T*)component;
}

template < typename T >
T* Registry::TryGetComponent( Entity entity )
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	const ae::Type* type = ae::GetType< T >();
	AE_ASSERT_MSG( type, "No registered type" );
	
	if ( ae::Map< Entity, Component* >* components = m_components.TryGet( type->GetId() ) )
	{
		return static_cast< T* >( components->Get( entity, nullptr ) );
	}
	
	return nullptr;
}

template < typename T >
T* Registry::TryGetComponent( const char* name )
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	return TryGetComponent< T >( m_entityNames.Get( name, kInvalidEntity ) );
}

template < typename T >
T& Registry::GetComponent( Entity entity )
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	T* t = TryGetComponent< T >( entity );
	AE_ASSERT( t );
	return *t;
}

template < typename T >
T& Registry::GetComponent( const char* name )
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	AE_ASSERT( name && name[ 0 ] );
	T* t = TryGetComponent< T >( name );
	if ( !t )
	{
		AE_ASSERT_MSG( GetEntityByName( name ) != kInvalidEntity, "No entity named '#'", name );
		AE_ASSERT_MSG( t, "No component '#' attached to entity '#'", ae::GetType< T >()->GetName(), name );
	}
	return *t;
}

template < typename T >
uint32_t Registry::GetComponentCount() const
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	const ae::Type* type = ae::GetType< T >();
	AE_ASSERT_MSG( type, "No registered type" );
	const ae::Map< Entity, Component* >* components = m_components.TryGet( type->GetId() );
	return components ? components->Length() : 0;
}

template < typename T >
Entity Registry::GetEntityByIndex( uint32_t index )
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	const ae::Type* type = ae::GetType< T >();
	AE_ASSERT_MSG( type, "No registered type" );
	const ae::Map< Entity, Component* >* components = m_components.TryGet( type->GetId() );
	AE_ASSERT_MSG( "No components of type '#'", type->GetName() );
	return components->GetKey( index );
}

template < typename T >
T& Registry::GetComponentByIndex( uint32_t index )
{
	AE_STATIC_ASSERT( (std::is_base_of< Component, T >::value) );
	const ae::Type* type = ae::GetType< T >();
	AE_ASSERT_MSG( type, "No registered type" );
	const ae::Map< Entity, Component* >* components = m_components.TryGet( type->GetId() );
	AE_ASSERT_MSG( "No components of type '#'", type->GetName() );
	return *(T*)components->GetValue( index );
}

template < typename T, typename Fn >
uint32_t Registry::CallFn( Fn fn )
{
	uint32_t result = 0;
	const ae::Type* type = ae::GetType< T >();
	for ( uint32_t i = 0; i < m_components.Length(); i++ )
	{
		const ae::Type* componentType = ae::GetTypeById( m_components.GetKey( i ) );
		if ( componentType->IsType( type ) )
		{
			// Get components each loop because m_components could grow at any iteration
			for ( uint32_t j = 0; j < m_components.GetValue( i ).Length(); j++ )
			{
				fn( (T*)m_components.GetValue( i ).GetValue( j ) );
				result++;
			}
		}
	}
	return result;
}

} // End ae namespace

#endif