//
// Game.h
//
#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Shader.h"
#include "modelclass.h"
#include "Light.h"
#include "Input.h"
#include "Camera.h"
#include "RenderTexture.h"
#include "Terrain.h"
#include "Cell.h"
#include "Grid.h"
#include "CellPoint.h"
#include "AStar.h"
#include "PostProcess.h"

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
public:

    Game() noexcept(false);
    ~Game();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);
#ifdef DXTK_AUDIO
    void NewAudioDevice();
#endif

    // Properties
    void GetDefaultSize( int& width, int& height ) const;
	
private:

	struct MatrixBufferType
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
	}; 

    void Update(DX::StepTimer const& timer);
    void Render();
    void Clear();
    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
	void SetupGUI();
	bool Intersect(ModelClass a, ModelClass b);
    bool IntersectCell(ModelClass a, Cell b);
    void SetPlayerOnGrid();

    // Minimap
    void RenderTextureMinimap();
    void Blur();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>    m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                           m_timer;

	//input manager. 
	Input									m_input;
	InputCommands							m_gameInputCommands;

    // DirectXTK objects.
    std::unique_ptr<DirectX::CommonStates>                                  m_states;
    std::unique_ptr<DirectX::BasicEffect>                                   m_batchEffect;	
    std::unique_ptr<DirectX::EffectFactory>                                 m_fxFactory;
    std::unique_ptr<DirectX::SpriteBatch>                                   m_sprites;
    std::unique_ptr<DirectX::SpriteFont>                                    m_font;

	// Scene Objects
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_batch;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>                               m_batchInputLayout;
	std::unique_ptr<DirectX::GeometricPrimitive>                            m_testmodel;

	//lights
	Light																	m_Light;

	//Cameras
	Camera																	m_Camera01;

	//textures 
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture1;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture2;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture3;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture4;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                        m_texture5;

	//Shaders
	Shader																	m_BasicShaderPair_Heightmap;
	Shader																	m_BasicShaderPair_NoHeightmap;

	//Scene. 
	Terrain																	m_Terrain;
	ModelClass																m_BasicModel;
	ModelClass																m_BasicModel2;
	ModelClass																m_BasicModel3;
    ModelClass                                                              m_Skybox;
    ModelClass																m_cellBox;
    ModelClass																m_playerBox;
    ModelClass                                                              m_treasureBox;

    //CellularAutomata
    Cell                                                                    m_Cell;
    Grid                                                                    m_Grid;

    //AStar
    AStar                                                                   m_AStar;

	//RenderTextures
	RenderTexture*															m_FirstRenderPass;
	RECT																	m_fullscreenRect;
	RECT																	m_CameraViewRect;
	


#ifdef DXTK_AUDIO
    std::unique_ptr<DirectX::AudioEngine>                                   m_audEngine;
    std::unique_ptr<DirectX::WaveBank>                                      m_waveBank;
    std::unique_ptr<DirectX::SoundEffect>                                   m_soundEffect;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect1;
    std::unique_ptr<DirectX::SoundEffectInstance>                           m_effect2;
#endif
    

#ifdef DXTK_AUDIO
    uint32_t                                                                m_audioEvent;
    float                                                                   m_audioTimerAcc;

    bool                                                                    m_retryDefault;
#endif

    DirectX::SimpleMath::Matrix                                             m_world;
    DirectX::SimpleMath::Matrix                                             m_view;
    DirectX::SimpleMath::Matrix                                             m_projection;
	bool																	isIntersecting;
    bool                                                                    m_playerInGrid;
    bool                                                                    m_isPlaying;
    bool                                                                    m_forwardCollision, m_backwardCollision;
    DirectX::SimpleMath::Vector3                                            m_currentPlayerPosition;
    DirectX::SimpleMath::Vector3                                            m_colliderPosition;
    DirectX::SimpleMath::Vector3                                            m_playerStartPosition;
    DirectX::SimpleMath::Vector3                                            m_playerStartRotation;
	std::wstring															debugLine;
    int                                                                     m_playerLives;
    int                                                                     m_playerCells;
    int                                                                     m_score;

    //BlurEffect
    bool																	enablePostProcess;
    std::unique_ptr<DirectX::BasicPostProcess>								m_postProcess;
    RenderTexture*                                                          m_RenderTexture;
    bool                                                                    minimap;
    bool                                                                    enableBlur;
};