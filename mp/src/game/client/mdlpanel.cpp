//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "matsys_controls/mdlpanel.h"
#include "materialsystem/imaterialsystem.h"
#include <materialsystem/IMaterialSystemHardwareConfig.h>
#include "materialsystem/imesh.h"
#include "vgui/ivgui.h"
#include "tier1/keyvalues.h"
#include "vgui_controls/frame.h"
#include "convar.h"
#include "tier0/dbg.h"
#include "istudiorender.h"
#include "matsys_controls/matsyscontrols.h"
#include "vcollide.h"
#include "vcollide_parse.h"
#include "bone_setup.h"
#include "vphysics_interface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_BUILD_FACTORY(CMDLPanel)

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CMDLPanel::CMDLPanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	SetVisible( true );

	// Used to poll input
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	// Deal with the default cubemap
	ITexture *pCubemapTexture = vgui::MaterialSystem()->FindTexture( "editor/cubemap", NULL, true );
	m_DefaultEnvCubemap.Init( pCubemapTexture );
	pCubemapTexture = vgui::MaterialSystem()->FindTexture( "editor/cubemap.hdr", NULL, true );
	m_DefaultHDREnvCubemap.Init( pCubemapTexture );

	SetIdentityMatrix(m_RootMDL.m_MDLToWorld);

	m_nNumSequenceLayers = 0;
	m_EventState.m_nEventSequence = -1;
	m_EventState.m_flPrevEventCycle = 0.0;
}

CMDLPanel::~CMDLPanel()
{
	m_DefaultEnvCubemap.Shutdown( );
	m_DefaultHDREnvCubemap.Shutdown();
}


//-----------------------------------------------------------------------------
// Scheme settings
//-----------------------------------------------------------------------------
void CMDLPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBorder( pScheme->GetBorder( "MenuBorder") );
}

/*
//-----------------------------------------------------------------------------
// DMEPanel..
//-----------------------------------------------------------------------------
void CMDLPanel::SetDmeElement( CDmeMDLMakefile *pMDLMakefile )
{
	CDisableUndoScopeGuard sg;

	if ( !m_bIsExternalMDL )
	{
		DestroyElement( m_hMDL );
		m_hMDL = NULL;
	}

	CDmeMDL *pMDL = NULL;
	if ( pMDLMakefile != NULL )
	{
		pMDL = CastElement< CDmeMDL >( pMDLMakefile->GetOutputElement( true ) );
		m_bIsExternalMDL = ( pMDL != NULL );
	}
	
	if ( !pMDL )
	{
		pMDL = CreateElement< CDmeMDL >( "mdl" );
	}

	m_hMDL = pMDL;
	m_hMDL->DrawInEngine( true );
	m_pDag->SetShape( pMDL );
}
*/

//-----------------------------------------------------------------------------
// Rendering options
//-----------------------------------------------------------------------------
void CMDLPanel::SetCollsionModel( bool bVisible )
{
	m_bDrawCollisionModel = bVisible;
}

void CMDLPanel::SetGroundGrid( bool bVisible )
{
	m_bGroundGrid = bVisible;
}

void CMDLPanel::SetWireFrame( bool bVisible )
{
	m_bWireFrame = bVisible;
}

void CMDLPanel::SetLockView( bool bLocked )
{
	m_bLockView = bLocked;
}

void CMDLPanel::SetLookAtCamera( bool bLookAtCamera )
{
	m_bLookAtCamera = bLookAtCamera;
}


//-----------------------------------------------------------------------------
// Stores the clip
//-----------------------------------------------------------------------------
void CMDLPanel::SetMDL( MDLHandle_t handle, void *pProxyData /* = NULL */ )
{
	m_RootMDL.m_MDL.SetMDL( handle );
}


//-----------------------------------------------------------------------------
// Sets the camera to look at the model
//-----------------------------------------------------------------------------
void CMDLPanel::LookAtMDL( )
{
	if ( m_bLockView )
		return;

	float flRadius;
	Vector vecCenter;
	GetMDLBoundingSphere(&vecCenter, &flRadius, m_RootMDL.m_MDL.GetMDL(), m_RootMDL.m_MDL.m_nSequence);
	LookAt( vecCenter, flRadius );
}


//-----------------------------------------------------------------------------
// FIXME: This should be moved into studiorender
//-----------------------------------------------------------------------------
static ConVar	r_showenvcubemap( "r_showenvcubemap", "0", FCVAR_CHEAT );
static ConVar	r_eyegloss		( "r_eyegloss", "1", FCVAR_ARCHIVE ); // wet eyes
static ConVar	r_eyemove		( "r_eyemove", "1", FCVAR_ARCHIVE ); // look around
static ConVar	r_eyeshift_x	( "r_eyeshift_x", "0", FCVAR_ARCHIVE ); // eye X position
static ConVar	r_eyeshift_y	( "r_eyeshift_y", "0", FCVAR_ARCHIVE ); // eye Y position
static ConVar	r_eyeshift_z	( "r_eyeshift_z", "0", FCVAR_ARCHIVE ); // eye Z position
static ConVar	r_eyesize		( "r_eyesize", "0", FCVAR_ARCHIVE ); // adjustment to iris textures
static ConVar	mat_softwareskin( "mat_softwareskin", "0" );
static ConVar	r_nohw			( "r_nohw", "0", FCVAR_CHEAT );
static ConVar	r_nosw			( "r_nosw", "0", FCVAR_CHEAT );
static ConVar	r_teeth			( "r_teeth", "1" );
static ConVar	r_drawentities	( "r_drawentities", "1", FCVAR_CHEAT );
static ConVar	r_flex			( "r_flex", "1" );
static ConVar	r_eyes			( "r_eyes", "1" );
static ConVar	r_skin			( "r_skin","0" );
static ConVar	r_maxmodeldecal ( "r_maxmodeldecal", "50" );
static ConVar	r_modelwireframedecal ( "r_modelwireframedecal", "0", FCVAR_CHEAT );
static ConVar	mat_normals		( "mat_normals", "0", FCVAR_CHEAT );
static ConVar	r_eyeglintlodpixels ( "r_eyeglintlodpixels", "0", FCVAR_CHEAT );
static ConVar	r_rootlod		( "r_rootlod", "0", FCVAR_CHEAT );

static StudioRenderConfig_t s_StudioRenderConfig;

void CMDLPanel::UpdateStudioRenderConfig( void )
{
	memset( &s_StudioRenderConfig, 0, sizeof(s_StudioRenderConfig) );

	s_StudioRenderConfig.bEyeMove = !!r_eyemove.GetInt();
	s_StudioRenderConfig.fEyeShiftX = r_eyeshift_x.GetFloat();
	s_StudioRenderConfig.fEyeShiftY = r_eyeshift_y.GetFloat();
	s_StudioRenderConfig.fEyeShiftZ = r_eyeshift_z.GetFloat();
	s_StudioRenderConfig.fEyeSize = r_eyesize.GetFloat();	
	if( mat_softwareskin.GetInt() || m_bWireFrame )
	{
		s_StudioRenderConfig.bSoftwareSkin = true;
	}
	else
	{
		s_StudioRenderConfig.bSoftwareSkin = false;
	}
	s_StudioRenderConfig.bNoHardware = !!r_nohw.GetInt();
	s_StudioRenderConfig.bNoSoftware = !!r_nosw.GetInt();
	s_StudioRenderConfig.bTeeth = !!r_teeth.GetInt();
	s_StudioRenderConfig.drawEntities = r_drawentities.GetInt();
	s_StudioRenderConfig.bFlex = !!r_flex.GetInt();
	s_StudioRenderConfig.bEyes = !!r_eyes.GetInt();
	s_StudioRenderConfig.bWireframe = m_bWireFrame;
	s_StudioRenderConfig.bDrawNormals = mat_normals.GetBool();
	s_StudioRenderConfig.skin = r_skin.GetInt();
	s_StudioRenderConfig.maxDecalsPerModel = r_maxmodeldecal.GetInt();
	s_StudioRenderConfig.bWireframeDecals = r_modelwireframedecal.GetInt() != 0;
	
	s_StudioRenderConfig.fullbright = false;
	s_StudioRenderConfig.bSoftwareLighting = false;

	s_StudioRenderConfig.bShowEnvCubemapOnly = r_showenvcubemap.GetInt() ? true : false;
	s_StudioRenderConfig.fEyeGlintPixelWidthLODThreshold = r_eyeglintlodpixels.GetFloat();

	StudioRender()->UpdateConfig( s_StudioRenderConfig );
}

void CMDLPanel::DrawCollisionModel()
{
	vcollide_t *pCollide = MDLCache()->GetVCollide( m_RootMDL.m_MDL.GetMDL() );

	if ( !pCollide || pCollide->solidCount <= 0 )
		return;
	
	static color32 color = {255,0,0,0};

	IVPhysicsKeyParser *pParser = g_pPhysicsCollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
	CStudioHdr studioHdr( g_pMDLCache->GetStudioHdr( m_RootMDL.m_MDL.GetMDL() ), g_pMDLCache );

	matrix3x4_t pBoneToWorld[MAXSTUDIOBONES];
	m_RootMDL.m_MDL.SetUpBones(m_RootMDL.m_MDLToWorld, MAXSTUDIOBONES, pBoneToWorld);

	// PERFORMANCE: Just parse the script each frame.  It's fast enough for tools.  If you need
	// this to go faster then cache off the bone index mapping in an array like HLMV does
	while ( !pParser->Finished() )
	{
		const char *pBlock = pParser->GetCurrentBlockName();
		if ( !stricmp( pBlock, "solid" ) )
		{
			solid_t solid;

			pParser->ParseSolid( &solid, NULL );
			int boneIndex = Studio_BoneIndexByName( &studioHdr, solid.name );
			Vector *outVerts;
			int vertCount = g_pPhysicsCollision->CreateDebugMesh( pCollide->solids[solid.index], &outVerts );

			if ( vertCount )
			{
				CMatRenderContextPtr pRenderContext( g_pMaterialSystem );
				pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );
				// NOTE: assumes these have been set up already by the model render code
				// So this is a little bit of a back door to a cache of the bones
				// this code wouldn't work unless you draw the model this frame before calling
				// this routine.  CMDLPanel always does this, but it's worth noting.
				// A better solution would be to move the ragdoll visulization into the CDmeMdl
				// and either draw it there or make it queryable and query/draw here.
				matrix3x4_t xform;
				SetIdentityMatrix( xform );
				if ( boneIndex >= 0 )
				{
					MatrixCopy( pBoneToWorld[ boneIndex ], xform );
				}
				IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_Wireframe );

				CMeshBuilder meshBuilder;
				meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, vertCount/3 );

				for ( int j = 0; j < vertCount; j++ )
				{
					Vector out;
					VectorTransform( outVerts[j].Base(), xform, out.Base() );
					meshBuilder.Position3fv( out.Base() );
					meshBuilder.Color4ub( color.r, color.g, color.b, color.a );
					meshBuilder.TexCoord2f( 0, 0, 0 );
					meshBuilder.AdvanceVertex();
				}
				meshBuilder.End();
				pMesh->Draw();
			}

			g_pPhysicsCollision->DestroyDebugMesh( vertCount, outVerts );
		}
		else
		{
			pParser->SkipBlock();
		}
	}
	g_pPhysicsCollision->VPhysicsKeyParserDestroy( pParser );
}

//-----------------------------------------------------------------------------
// paint it!
//-----------------------------------------------------------------------------
void CMDLPanel::OnPaint3D()
{
	if ( !m_RootMDL.m_MDL.GetMDL() )
		return;

	// FIXME: Move this call into DrawModel in StudioRender
	StudioRenderConfig_t oldStudioRenderConfig;
	StudioRender()->GetCurrentConfig( oldStudioRenderConfig );

	UpdateStudioRenderConfig();

	CMatRenderContextPtr pRenderContext( vgui::MaterialSystem() );
	ITexture *pLocalCube = pRenderContext->GetLocalCubemap();
	if ( vgui::MaterialSystemHardwareConfig()->GetHDRType() == HDR_TYPE_NONE )
	{
		ITexture *pMyCube = HasLightProbe() ? GetLightProbeCubemap( false ) : m_DefaultEnvCubemap;
		pRenderContext->BindLocalCubemap( pMyCube );
	}
	else
	{
		ITexture *pMyCube = HasLightProbe() ? GetLightProbeCubemap( true ) : m_DefaultHDREnvCubemap;
		pRenderContext->BindLocalCubemap( pMyCube );
	}

	if ( m_bGroundGrid )
	{
		DrawGrid();
	}

	if ( m_bLookAtCamera )
	{
		matrix3x4_t worldToCamera;
		ComputeCameraTransform( &worldToCamera );

		Vector vecPosition;
		MatrixGetColumn( worldToCamera, 3, vecPosition );
		m_RootMDL.m_MDL.m_bWorldSpaceViewTarget = true;
		m_RootMDL.m_MDL.m_vecViewTarget = vecPosition;
	}

	// Draw the MDL
	
	m_RootMDL.m_MDL.Draw(m_RootMDL.m_MDLToWorld);
	
	if ( m_bDrawCollisionModel )
	{
		DrawCollisionModel();
	}

	pRenderContext->Flush();
	StudioRender()->UpdateConfig( oldStudioRenderConfig );
	pRenderContext->BindLocalCubemap( pLocalCube );
}


//-----------------------------------------------------------------------------
// Sets the current sequence
//-----------------------------------------------------------------------------
void CMDLPanel::SetSequence( int nSequence, bool bResetSequence /* = false */ )
{
	m_RootMDL.m_MDL.m_nSequence = nSequence;
}

void CMDLPanel::SetSkin( int nSkin )
{
	m_RootMDL.m_MDL.m_nSkin = nSkin;
}

	
//-----------------------------------------------------------------------------
// Purpose: Keeps a global clock to autoplay sequences to run from
//			Also deals with speedScale changes
//-----------------------------------------------------------------------------
float GetAutoPlayTime( void )
{
	static int g_prevTicks;
	static float g_time;

	int ticks = Plat_MSTime();

	// limit delta so that float time doesn't overflow
	if (g_prevTicks == 0)
	{
		g_prevTicks = ticks;
	}

	g_time += ( ticks - g_prevTicks ) / 1000.0f;
	g_prevTicks = ticks;

	return g_time;
}


//-----------------------------------------------------------------------------
// called when we're ticked...
//-----------------------------------------------------------------------------
void CMDLPanel::OnTick()
{
	BaseClass::OnTick();
	if ( m_RootMDL.m_MDL.GetMDL() != MDLHANDLE_INVALID)
	{
		m_RootMDL.m_MDL.m_flTime = GetAutoPlayTime();
	}
}

//-----------------------------------------------------------------------------
// input
//-----------------------------------------------------------------------------
void CMDLPanel::OnMouseDoublePressed( vgui::MouseCode code )
{
	float flRadius;
	Vector vecCenter;
	GetMDLBoundingSphere(&vecCenter, &flRadius, m_RootMDL.m_MDL.GetMDL(), m_RootMDL.m_MDL.m_nSequence);
	LookAt( vecCenter, flRadius );

	BaseClass::OnMouseDoublePressed( code );
}

/*
//-----------------------------------------------------------------------------
// Draws the grid under the MDL
//-----------------------------------------------------------------------------
void CMDLPanel::DrawGrid()
{
	matrix3x4_t transform;

	SetIdentityMatrix( transform );

	CMatRenderContextPtr pRenderContext( MaterialSystem() );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->LoadMatrix( transform );

	pRenderContext->Bind( m_Wireframe );

	IMesh *pMesh = pRenderContext->GetDynamicMesh();

	int nGridDim = 10;
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_LINES, 2 * nGridDim + 2 );

	float bounds = 100.0f;
	float delta = 2 * bounds / nGridDim;
	for ( int i = 0; i < nGridDim + 1; ++i )
	{
		float xy = -bounds + delta * i;

		meshBuilder.Position3f( xy, -bounds, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();
		meshBuilder.Position3f( xy, bounds, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3f( -bounds, xy, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();
		meshBuilder.Position3f( bounds, xy, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	pMesh->Draw();
}
*/