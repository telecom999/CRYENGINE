#include "StdAfx.h"
#include "EnvironmentProbeComponent.h"

#include <Cry3DEngine/IRenderNode.h>

namespace Cry
{
	namespace DefaultComponents
	{
		static void RegisterEnvironmentProbeComponent(Schematyc::IEnvRegistrar& registrar)
		{
			Schematyc::CEnvRegistrationScope scope = registrar.Scope(IEntity::GetEntityScopeGUID());
			{
				Schematyc::CEnvRegistrationScope componentScope = scope.Register(SCHEMATYC_MAKE_ENV_COMPONENT(CEnvironmentProbeComponent));
				// Functions
				{
					auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEnvironmentProbeComponent::LoadFromDisk, "122049AA-015F-4F30-933D-BF2E7C6BA0BC"_cry_guid, "LoadFromDisk");
					pFunction->SetDescription("Loads a cube map texture from disk and applies it");
					pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
					pFunction->BindInput(1, 'path', "Cube map Texture Path");
					componentScope.Register(pFunction);
				}
				{
					auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEnvironmentProbeComponent::SetCubemap, "F305D0C4-6AD2-4FD8-93A9-330A91206360"_cry_guid, "SetCubemap");
					pFunction->SetDescription("Sets the cube map from already loaded textures");
					pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
					pFunction->BindInput(1, 'spec', "Specular cube map texture id");
					pFunction->BindInput(2, 'diff', "Diffuse cube map texture id");
					componentScope.Register(pFunction);
				}
#ifdef SUPPORT_ENVIRONMENT_PROBE_GENERATION
				{
					auto pFunction = SCHEMATYC_MAKE_ENV_FUNCTION(&CEnvironmentProbeComponent::Generate, "024A11F7-8C74-493A-A0A7-3D613281BEDE"_cry_guid, "Generate");
					pFunction->SetDescription("Generates the cubemap to the specified path");
					pFunction->SetFlags(Schematyc::EEnvFunctionFlags::Construction);
					pFunction->BindInput(1, 'path', "Output Texture Path");
					componentScope.Register(pFunction);
				}
#endif
			}
		}

		void CEnvironmentProbeComponent::ReflectType(Schematyc::CTypeDesc<CEnvironmentProbeComponent>& desc)
		{
			desc.SetGUID(CEnvironmentProbeComponent::IID());
			desc.SetEditorCategory("Lights");
			desc.SetLabel("Environment Probe");
			desc.SetDescription("Captures an image of its full surroundings and used to light nearby objects with reflective materials");
			desc.SetIcon("icons:Designer/Designer_Box.ico");
			desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

			desc.AddMember(&CEnvironmentProbeComponent::m_bActive, 'actv', "Active", "Active", "Determines whether the environment probe is enabled", true);
			desc.AddMember(&CEnvironmentProbeComponent::m_extents, 'exts', "BoxSize", "Box Size", "Size of the area the probe affects.", Vec3(10.f));

			desc.AddMember(&CEnvironmentProbeComponent::m_options, 'opt', "Options", "Options", "Specific Probe Options", CEnvironmentProbeComponent::SOptions());
			desc.AddMember(&CEnvironmentProbeComponent::m_color, 'colo', "Color", "Color", "Probe Color emission information", CEnvironmentProbeComponent::SColor());
			desc.AddMember(&CEnvironmentProbeComponent::m_generation, 'gen', "Generation", "Generation Parameters", "Parameters for default cube map generation and load", CEnvironmentProbeComponent::SGeneration());
		}

#ifdef SUPPORT_ENVIRONMENT_PROBE_GENERATION
		static void ReflectType(Schematyc::CTypeDesc<CEnvironmentProbeComponent::SGeneration::EResolution>& desc)
		{
			desc.SetGUID("204041D6-A198-4DAA-B8B0-4D034DF849BD"_cry_guid);
			desc.SetLabel("Cube map Resolution");
			desc.SetDescription("Resolution to render a cube map at");
			desc.SetDefaultValue(CEnvironmentProbeComponent::SGeneration::EResolution::x256);
			desc.AddConstant(CEnvironmentProbeComponent::SGeneration::EResolution::x256, "Low", "256");
			desc.AddConstant(CEnvironmentProbeComponent::SGeneration::EResolution::x512, "Medium", "512");
			desc.AddConstant(CEnvironmentProbeComponent::SGeneration::EResolution::x1024, "High", "1024");
		}
#endif

		static void ReflectType(Schematyc::CTypeDesc<CEnvironmentProbeComponent::SOptions>& desc)
		{
			desc.SetGUID("{DB10AB64-7A5B-4B91-BC90-6D692D1D1222}"_cry_guid);
			desc.AddMember(&CEnvironmentProbeComponent::SOptions::m_bIgnoreVisAreas, 'igvi', "IgnoreVisAreas", "Ignore VisAreas", nullptr, false);
			desc.AddMember(&CEnvironmentProbeComponent::SOptions::sortPriority, 'spri', "SortPriority", "Sort Priority", nullptr, (uint32)CDLight().m_nSortPriority);
			desc.AddMember(&CEnvironmentProbeComponent::SOptions::m_attenuationFalloffMax, 'atte', "AttenuationFalloffMax", "Maximum Attenuation Falloff", nullptr, 1.f);
			desc.AddMember(&CEnvironmentProbeComponent::SOptions::m_bAffectsVolumetricFog, 'volf', "AffectVolumetricFog", "Affect Volumetric Fog", nullptr, true);
			desc.AddMember(&CEnvironmentProbeComponent::SOptions::m_bVolumetricFogOnly, 'volo', "VolumetricFogOnly", "Only Affect Volumetric Fog", nullptr, false);
			desc.AddMember(&CEnvironmentProbeComponent::SOptions::m_bAffectsOnlyThisArea, 'area', "OnlyAffectThisArea", "Only Affect This Area", nullptr, true);
			desc.AddMember(&CEnvironmentProbeComponent::SOptions::m_bBoxProjection, 'boxp', "BoxProjection", "Use Box Projection", nullptr, false);
		}

		static void ReflectType(Schematyc::CTypeDesc<CEnvironmentProbeComponent::SColor>& desc)
		{
			desc.SetGUID("{B71C3414-F85A-4EAA-9CE0-5110A2E040AD}"_cry_guid);
			desc.AddMember(&CEnvironmentProbeComponent::SColor::m_color, 'col', "Color", "Color", nullptr, ColorF(1.f));
			desc.AddMember(&CEnvironmentProbeComponent::SColor::m_diffuseMultiplier, 'diff', "DiffMult", "Diffuse Multiplier", nullptr, 1.f);
			desc.AddMember(&CEnvironmentProbeComponent::SColor::m_specularMultiplier, 'spec', "SpecMult", "Specular Multiplier", nullptr, 1.f);
		}

		static void ReflectType(Schematyc::CTypeDesc<CEnvironmentProbeComponent::SGeneration>& desc)
		{
			desc.SetGUID("{1CF55472-FAD0-42E7-B06A-20B96F954914}"_cry_guid);
			desc.AddMember(&CEnvironmentProbeComponent::SGeneration::m_generatedCubemapPath, 'tex', "GeneratedCubemapPath", "Cube Map Path", nullptr);
			desc.AddMember(&CEnvironmentProbeComponent::SGeneration::m_bAutoLoad, 'auto', "AutoLoad", "Load Texture Automatically", "If set, tries to load the cube map texture specified on component creation", true);

#ifdef SUPPORT_ENVIRONMENT_PROBE_GENERATION
			desc.AddMember(&CEnvironmentProbeComponent::SGeneration::m_resolution, 'res', "Resolution", "Resolution", "Resolution to render cube map at", CEnvironmentProbeComponent::SGeneration::EResolution::x256);
			desc.AddMember(&CEnvironmentProbeComponent::SGeneration::m_generateButton, 'btn', "Generate", "Generate", "Generates the cube map");
#endif
		}

		void CEnvironmentProbeComponent::Run(Schematyc::ESimulationMode simulationMode)
		{
			Initialize();
		}

		void CEnvironmentProbeComponent::SetOutputCubemapPath(const char* szPath)
		{
			m_generation.m_generatedCubemapPath = szPath;
		}
	}
}