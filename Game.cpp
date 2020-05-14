//
// Game.cpp
//

#include "pch.h"
#include "Game.h"


//toreorganise
#include <fstream>

#define MAX_LIVES 3

extern void ExitGame();

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace ImGui;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
	m_deviceResources->RegisterDeviceNotify(this);
}

Game::~Game()
{
#ifdef DXTK_AUDIO
    if (m_audEngine)
    {
        m_audEngine->Suspend();
    }
#endif
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{

	m_input.Initialise(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

	//setup imgui.  its up here cos we need the window handle too
	//pulled from imgui directx11 example
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window);		//tie to our window // hook it to window
	ImGui_ImplDX11_Init(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext());	//tie to directx

	m_fullscreenRect.left = 20;
	m_fullscreenRect.top = 20;
	m_fullscreenRect.right = 500;
	m_fullscreenRect.bottom = 300;

	m_CameraViewRect.left = 500;
	m_CameraViewRect.top = 0;
	m_CameraViewRect.right = 800;
	m_CameraViewRect.bottom = 240;

	//setup light
	m_Light.setAmbientColour(0.3f, 0.3f, 0.3f, 1.0f);
	m_Light.setDiffuseColour(1.0f, 1.0f, 1.0f, 1.0f);
	m_Light.setPosition(12.0f, 1.0f, 12.0f);
	m_Light.setDirection(-1.0f, -1.0f, 0.0f);

	// Variable Initialisation
	isIntersecting = false;
	m_playerInGrid = false;
	m_isPlaying = false;
	minimap = false;
	enableBlur = false;
	m_score = 0;
	m_playerCells = 0;
	
	// CellGrid search for player // move to separate function for level generation?

	SetPlayerOnGrid();

	//setup camera
	//m_Camera01.setPosition(Vector3(0.0f, 2.0f, 0.0f)); // m_Grid.GetCameraCellPosition();
	//m_Camera01.setPosition(m_Grid.GetPlayerCell().GetPosition());
	//m_Camera01.setRotation(Vector3(-90.0f, -180.0f, 0.0f));	//orientation is -90 becuase zero will be looking up at the sky straight up. 

	
	
#ifdef DXTK_AUDIO
    // Create DirectXTK for Audio objects
    AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
    eflags = eflags | AudioEngine_Debug;
#endif

    m_audEngine = std::make_unique<AudioEngine>(eflags);

    m_audioEvent = 0;
    m_audioTimerAcc = 10.f;
    m_retryDefault = false;

    m_waveBank = std::make_unique<WaveBank>(m_audEngine.get(), L"adpcmdroid.xwb");

    m_soundEffect = std::make_unique<SoundEffect>(m_audEngine.get(), L"MusicMono_adpcm.wav");
    m_effect1 = m_soundEffect->CreateInstance();
    m_effect2 = m_waveBank->CreateInstance(10);

    m_effect1->Play(true);
    m_effect2->Play();
#endif

}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
	//take in input
	m_input.Update();								//update the hardware
	m_gameInputCommands = m_input.getGameInput();	//retrieve the input for our game
	
	//Update all game objects
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

	//Render all game content. 
    Render();

#ifdef DXTK_AUDIO
    // Only update audio engine once per frame
    if (!m_audEngine->IsCriticalError() && m_audEngine->Update())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
#endif

	
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{	
	// Time.deltaTime equivelant
	float deltaTime = float(timer.GetElapsedSeconds());
	
	//this is hacky,  i dont like this here.  
	auto device = m_deviceResources->GetD3DDevice();

	//note that currently.  Delta-time is not considered in the game object movement. 
	// camera is the player
	/* Rotation */
	if (m_gameInputCommands.left)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.y = rotation.y -= m_Camera01.getRotationSpeed();
		m_Camera01.setRotation(rotation);
	}
	if (m_gameInputCommands.right)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.y = rotation.y += m_Camera01.getRotationSpeed();
		m_Camera01.setRotation(rotation);
	}
	/* Movement */
	if (!isIntersecting)
	{
		if (m_gameInputCommands.forward)
		{
			Vector3 position = m_Camera01.getPosition(); //get the position
			position += (m_Camera01.getForward() * m_Camera01.getMoveSpeed()); //add the forward vector
			if (position.x >= 25.f) {
				position.x = 25.f;
			}
			if (position.x <= 0.f) {
				position.x = 0.f;
			}
			if (position.z >= 25.f)
				position.z = 25.0f;
			if (position.z <= 0.f)
				position.z = 0.f;
			m_Camera01.setPosition(position);
		}
		if (m_gameInputCommands.back)
		{
			Vector3 position = m_Camera01.getPosition(); //get the position
			position -= (m_Camera01.getForward() * m_Camera01.getMoveSpeed()); //add the forward vector
			if (position.x >= 25.f) {
				position.x = 25.f;
			}
			if (position.x <= 0.f) {
				position.x = 0.f;
			}
			if (position.z >= 25.f)
				position.z = 25.0f;
			if (position.z <= 0.f)
				position.z = 0.f;
			m_Camera01.setPosition(position);
		}
		m_currentPlayerPosition = m_Camera01.getPosition();
	}
	else {
		// if you intersect reset player position to playerCell
		Vector3 newPosition = Vector3(m_colliderPosition.x + 0.5f, 0.5f, m_colliderPosition.z + 0.5f);
		m_Camera01.setPosition(m_playerStartPosition); //m_colliderPosition + m_currentPlayerPosition
		m_Camera01.setRotation(m_playerStartRotation);
		if (m_score <= 0) 
		{
			m_score = 0;
		}
		else {
			m_score -= 10;
		}
		isIntersecting = false;
		/*
		if (m_forwardCollision) 
		{
			m_Camera01.setPosition(Vector3(10,10,10));
		}
		if (m_backwardCollision)
		{
			//newPosition = m_colliderPosition + new 
			
		}
		*/
	}

	if (m_gameInputCommands.up)
	{
		Vector3 position = m_Camera01.getPosition(); //get the position
		position.y = position.y += m_Camera01.getMoveSpeed(); //add the forward vector
		if (position.y >= 10.f) position.y = 10.0f;
		m_Camera01.setPosition(position);
	}
	if (m_gameInputCommands.down)
	{
		Vector3 position = m_Camera01.getPosition(); //get the position
		position.y = position.y -= m_Camera01.getMoveSpeed(); //add the forward vector
		if (position.y <= 2.0f) position.y = 2.0f;
		m_Camera01.setPosition(position);
	}
	/* Removed for final game */
	if (m_gameInputCommands.lookUp)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.x = rotation.x += m_Camera01.getRotationSpeed();
		if (rotation.x >= -10.0f) rotation.x = -10.f;
		m_Camera01.setRotation(rotation);
	}
	if (m_gameInputCommands.lookDown)
	{
		Vector3 rotation = m_Camera01.getRotation();
		rotation.x = rotation.x -= m_Camera01.getRotationSpeed();
		if (rotation.x <= -170.f) rotation.x = -170.f;
		m_Camera01.setRotation(rotation);
	}

	if (m_gameInputCommands.generate)
	{
		m_Terrain.GenerateHeightMap(device);
	}

	if (m_gameInputCommands.midpoint) {
		m_Terrain.GenerateMidpointHeightMap(device);
	}
	
	if (m_gameInputCommands.smoothen) {
		m_Terrain.SmoothenHeightMap(device);
	}
	
	if (m_gameInputCommands.automata) {
		m_Grid.nextGeneration();
	}
	
	if (m_gameInputCommands.minimap) {
		minimap = !minimap;
	}

	if (m_gameInputCommands.bloom) {
		enableBlur = !enableBlur;
	}

	m_Camera01.Update();	//camera update.
	
	m_Terrain.Update();		//terrain update.  doesnt do anything at the moment. 

	m_view = m_Camera01.getCameraMatrix();
	m_world = Matrix::Identity;

	/*create our UI*/ // Toggle on / off gui
	SetupGUI();
	
	std::wstring ws2 = L" Camera Position";
	std::wstring ws1 = std::to_wstring(m_playerCells);
	std::wstring s(ws1);
	s += std::wstring(ws2);
	const wchar_t* debug = s.c_str();
	debugLine = debug;
	
	// treasure box collision check
	if (Intersect(m_playerBox, m_treasureBox)) 
	{
		debugLine = L"Treasure Box Collected!";
		m_Grid.initializeGrid();
		SetPlayerOnGrid();
		m_score += 20;
		//m_Grid.initializeGrid(); // take playerbox and treasure box
		// initialize grid should also be fed to A Star
	}
	else {
		debugLine = L"";
		isIntersecting = false;
	}
	// Check intersection with each cell in grid

	// Collision check
	if (m_Grid.GetInitialised()) { // update happens before render, so this ensures render goes through first .. not sure about this check where grid initialisation is happening 
								  // get set .. set when has finished rendering down below ..		
		for (int i = 0; i < m_Grid.Size(); i++)
		{
			for (int j = 0; j < m_Grid.Size(); j++)
			{
				if (IntersectCell(m_playerBox, m_Grid.cellMatrix[i][j]))
				{
					if (m_Grid.cellMatrix[i][j].GetState() == 1)
					{
						// Several states all at once // 
						
						//if (m_colliderPosition != m_Grid.cellMatrix[i][j].GetPosition())
						//{
						m_colliderPosition = m_Grid.cellMatrix[i][j].GetPosition(); // needs to be used
						//}
						if (m_colliderPosition == m_playerStartPosition)
						{
							m_Grid.initializeGrid();
							SetPlayerOnGrid();
							m_playerLives = MAX_LIVES;
						}
						else {
							debugLine = L"Hit Wall!";
							isIntersecting = true;
						}
					}
					else {
						//m_collisions--;
						//debugLine = L"Collision Null";
					}
					//Intersect(m_playerBox, m_cellBox) ? isIntersecting = true : isIntersecting = false;
					//isIntersecting ? debugLine = L"Collision Detected" : debugLine = L"Collision Null";
					//debugLine = L"Collision Detected";
				}
				else {
					m_isPlaying = true;
					m_playerLives = MAX_LIVES;
					//debugLine = L"Null";
					//m_collisions--;
					//debugLine = L"Collision Null";
					//m_collisions--;
					//debugLine = L"No Collision";
				}
			}
		}
		
	}
	//debugLine = L"";
	//debugLine = m_Camera01.getPosition().x;
	//debugLine += m_Camera01.getPosition().z;
	
	

	// Check intersection between two boxes
	//Intersect(m_BoxOne, m_BoxTwo) ? debugLine = L"Collision Detected" : debugLine = L"Collision Null";
	
#ifdef DXTK_AUDIO
    m_audioTimerAcc -= (float)timer.GetElapsedSeconds();
    if (m_audioTimerAcc < 0)
    {
        if (m_retryDefault)
        {
            m_retryDefault = false;
            if (m_audEngine->Reset())
            {
                // Restart looping audio
                m_effect1->Play(true);
            }
        }
        else
        {
            m_audioTimerAcc = 4.f;

            m_waveBank->Play(m_audioEvent++);

            if (m_audioEvent >= 11)
                m_audioEvent = 0;
        }
    }
#endif

  
	if (m_input.Quit())
	{
		ExitGame();
	}
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{	
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();

	// Draw skybox here // translate to camera position

	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthNone(), 0);
	context->RSSetState(m_states->CullNone());

	m_world = SimpleMath::Matrix::Identity;
	SimpleMath::Matrix skyboxPosition = SimpleMath::Matrix::CreateTranslation(m_Camera01.getPosition());


	m_world = m_world * skyboxPosition;
	// Turn our shaders on,  set parameters // MuseumFloor
	m_BasicShaderPair_NoHeightmap.EnableShader(context);
	m_BasicShaderPair_NoHeightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture5.Get());
	//render our model
	m_Skybox.Render(context);

	//Reset Rendering states. 
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullClockwise());
	//context->RSSetState(m_states->Wireframe()); // this enables wireframe

	// Minimap Render to Texture
	RenderTextureMinimap();

	if (enableBlur)
	{
		Blur();
	}
	else {
		//prepare transform for floor object. 
		m_world = SimpleMath::Matrix::Identity; //set world back to identity
		SimpleMath::Matrix newPosition3 = SimpleMath::Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
		SimpleMath::Matrix newScale = SimpleMath::Matrix::CreateScale(0.1);		//scale the terrain down a little. 
		m_world = m_world * newScale * newPosition3;

		//setup and draw floor
		m_BasicShaderPair_Heightmap.EnableShader(context);
		m_BasicShaderPair_Heightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
		m_Terrain.Render(context);

		// CellGrid

		for (int i = 0; i < m_Grid.Size(); i++)
		{
			for (int j = 0; j < m_Grid.Size(); j++)
			{

				Vector3 tempPosition = DirectX::SimpleMath::Vector3(i, 0.5f, j);
				m_Grid.cellMatrix[i][j].SetPosition(tempPosition);
				m_Grid.cellMatrix[i][j].SetCentre(tempPosition);

				m_world = SimpleMath::Matrix::Identity; //set world back to identity
				newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Grid.cellMatrix[i][j].GetPosition());
				m_world = m_world * newPosition3;

				if (m_Grid.cellMatrix[i][j].GetState() == 1)
				{
					m_BasicShaderPair_NoHeightmap.EnableShader(context);
					m_BasicShaderPair_NoHeightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture2.Get());
					m_cellBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
					m_cellBox.Render(context);
				}
				else if (m_Grid.cellMatrix[i][j].GetState() == 3)
				{
					//setup and draw treasure // draw object
					m_BasicShaderPair_NoHeightmap.EnableShader(context);
					m_BasicShaderPair_NoHeightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture3.Get());
					m_treasureBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
					m_treasureBox.Render(context);
				}
				else {
					// do nothing
				}
			}
		}


		/* First render player so that Intersect will know beforehand where player is in current point of time */

		//prepare transform for Player object. // set coordinates
		m_world = SimpleMath::Matrix::Identity; //set world back to identity
		newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Camera01.getPosition().x, m_Camera01.getPosition().y, m_Camera01.getPosition().z);
		m_playerBox.SetCentre(m_Camera01.getPosition());
		newScale = SimpleMath::Matrix::CreateScale(1);
		m_world = m_world * newScale * newPosition3;

		//setup and draw rectangle
		m_BasicShaderPair_NoHeightmap.EnableShader(context);
		m_BasicShaderPair_NoHeightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, NULL); // invisible collision box for player
		m_playerBox.Render(context);
	}
	

	if (!m_Grid.GetInitialised()) {
		m_Grid.SetInitialised(true);
	}

	//render our GUI
	//ImGui::Render();
	//ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	// Hides GUI
	ImGui::EndFrame();

	/* FPS */
	std::wstring ws2 = L" FPS";
	std::wstring ws1 = std::to_wstring(m_timer.GetFramesPerSecond());
	std::wstring s_fps(ws1);
	s_fps += std::wstring(ws2);
	const wchar_t* fps = s_fps.c_str();

	/* PlayerScore */
	std::wstring ws4 = L" points";
	std::wstring ws3 = std::to_wstring(m_score);
	std::wstring s_playerScore(ws3);
	s_playerScore += std::wstring(ws4);
	const wchar_t* playerScore = s_playerScore.c_str();

	/* Distance to Treasure */
	std::wstring ws5 = std::to_wstring(*m_Grid.GetDistance());
	std::wstring ws6 = L" cells";
	std::wstring s_treasureDistance(ws5);
	s_treasureDistance += std::wstring(ws6);
	const wchar_t* distance = s_treasureDistance.c_str();

	// Draw Text to the screen
	m_sprites->Begin();
	m_font->DrawString(m_sprites.get(), L"Find the treasure!", XMFLOAT2((m_deviceResources->GetScreenViewport().Width / 2) - 100.0f, m_deviceResources->GetScreenViewport().TopLeftY), Colors::Yellow);
	if (*m_Grid.GetDistance() != 0)
	{
		//m_font->DrawString(m_sprites.get(), distance, XMFLOAT2((m_deviceResources->GetScreenViewport().Width) - 100.0f, m_deviceResources->GetScreenViewport().TopLeftY), Colors::Yellow);
	}
	else 
	{
		//m_font->DrawString(m_sprites.get(), L"Press G", XMFLOAT2((m_deviceResources->GetScreenViewport().Width) - 100.0f, m_deviceResources->GetScreenViewport().TopLeftY), Colors::Yellow);
	}
	m_font->DrawString(m_sprites.get(), fps, XMFLOAT2(5, m_deviceResources->GetScreenViewport().Height - 40.f), Colors::White);
	m_font->DrawString(m_sprites.get(), L"G - Next iteration in Cellular Automata | V - Render to Texture | B - Blur", XMFLOAT2((m_deviceResources->GetScreenViewport().Width / 2) - 400.0f, m_deviceResources->GetScreenViewport().Height - 40.f), Colors::Yellow);
	m_font->DrawString(m_sprites.get(), playerScore, XMFLOAT2(m_deviceResources->GetScreenViewport().Width - 110.0f, m_deviceResources->GetScreenViewport().Height - 40.f), Colors::White);
	//m_font->DrawString(m_sprites.get(), debugLine.c_str(), XMFLOAT2(10, 40), Colors::Yellow);
	m_sprites->End();

	// Draw our sprite with the render texture displayed on it.
	if (minimap)
	{
		m_sprites->Begin();
		m_sprites->Draw(m_FirstRenderPass->getShaderResourceView(), m_fullscreenRect);
		m_sprites->End();
	}

    // Show the new frame.
    m_deviceResources->Present();
}

void Game::RenderTextureMinimap() {

	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();
	// Set the render target to be the render to texture.
	m_FirstRenderPass->setRenderTarget(context);
	// Clear the render to texture.
	m_FirstRenderPass->clearRenderTarget(context, 0.8f, 0.8f, 0.8f, 1.0f);

	//prepare transform for floor object. 
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	SimpleMath::Matrix newPosition3 = SimpleMath::Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
	SimpleMath::Matrix newScale = SimpleMath::Matrix::CreateScale(0.1);		//scale the terrain down a little. 
	m_world = m_world * newScale * newPosition3;

	//setup and draw floor
	m_BasicShaderPair_Heightmap.EnableShader(context);
	m_BasicShaderPair_Heightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
	m_Terrain.Render(context);

	// CellGrid

	for (int i = 0; i < m_Grid.Size(); i++)
	{
		for (int j = 0; j < m_Grid.Size(); j++)
		{

			Vector3 tempPosition = DirectX::SimpleMath::Vector3(i, 0.5f, j);
			m_Grid.cellMatrix[i][j].SetPosition(tempPosition);
			m_Grid.cellMatrix[i][j].SetCentre(tempPosition);

			m_world = SimpleMath::Matrix::Identity; //set world back to identity
			newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Grid.cellMatrix[i][j].GetPosition());
			m_world = m_world * newPosition3;

			if (m_Grid.cellMatrix[i][j].GetState() == 1)
			{
				m_BasicShaderPair_NoHeightmap.EnableShader(context);
				m_BasicShaderPair_NoHeightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture2.Get());
				m_cellBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
				m_cellBox.Render(context);
			}
			else if (m_Grid.cellMatrix[i][j].GetState() == 3)
			{
				//setup and draw treasure // draw object
				m_BasicShaderPair_NoHeightmap.EnableShader(context);
				m_BasicShaderPair_NoHeightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture3.Get());
				m_treasureBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
				m_treasureBox.Render(context);
			}
			else {
				// do nothing
			}
		}
	}

	/* First render player so that Intersect will know beforehand where player is in current point of time */

	//prepare transform for Player object. // set coordinates
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Camera01.getPosition().x, m_Camera01.getPosition().y, m_Camera01.getPosition().z);
	m_playerBox.SetCentre(m_Camera01.getPosition());
	newScale = SimpleMath::Matrix::CreateScale(1);
	m_world = m_world * newScale * newPosition3;

	//setup and draw rectangle
	m_BasicShaderPair_NoHeightmap.EnableShader(context);
	m_BasicShaderPair_NoHeightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture4.Get()); // show texture for minimap
	m_playerBox.Render(context);

	// Reset the render target back to the original back buffer and not the render to texture anymore.	
	context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);
}

bool Game::Intersect(ModelClass a, ModelClass b) 
{
	float aMinX, aMaxX, aMinY, aMaxY, aMinZ, aMaxZ = 0.f;
	float bMinX, bMaxX, bMinY, bMaxY, bMinZ, bMaxZ = 0.f;

	float widthA = a.GetDimensions().x;
	float widthB = b.GetDimensions().x;
	float heightA = a.GetDimensions().y;
	float heightB = b.GetDimensions().y;
	float depthA = a.GetDimensions().z;
	float depthB = b.GetDimensions().z;

	aMinX = a.GetCentre().x - (widthA / 2);
	aMaxX = a.GetCentre().x + (widthA / 2);

	aMinY = a.GetCentre().y - (heightA / 2);
	aMaxY = a.GetCentre().y + (heightA / 2);

	aMinZ = a.GetCentre().z - (depthA / 2);
	aMaxZ = a.GetCentre().z + (depthA / 2);

	bMinX = b.GetCentre().x - (widthB / 2);
	bMaxX = b.GetCentre().x + (widthB / 2);

	bMinY = b.GetCentre().y - (heightB / 2);
	bMaxY = b.GetCentre().y + (heightB / 2);

	bMinZ = b.GetCentre().z - (depthB / 2);
	bMaxZ = b.GetCentre().z + (depthB / 2);

	// Get two boxes and check collisions between them AABB
	return (aMinX <= bMaxX && aMaxX >= bMinX) &&
		   (aMinY <= bMaxY && aMaxY >= bMinY) &&
		   (aMinZ <= bMaxZ && aMaxZ >= bMinZ);
}

bool Game::IntersectCell(ModelClass a, Cell b)
{
	float aMinX, aMaxX, aMinY, aMaxY, aMinZ, aMaxZ = 0.f;
	float bMinX, bMaxX, bMinY, bMaxY, bMinZ, bMaxZ = 0.f;

	float widthA = a.GetDimensions().x;
	float widthB = b.GetDimensions().x;
	float heightA = a.GetDimensions().y;
	float heightB = b.GetDimensions().y;
	float depthA = a.GetDimensions().z;
	float depthB = b.GetDimensions().z;

	aMinX = a.GetCentre().x - (widthA / 2);
	aMaxX = a.GetCentre().x + (widthA / 2);

	aMinY = a.GetCentre().y - (heightA / 2);
	aMaxY = a.GetCentre().y + (heightA / 2);

	aMinZ = a.GetCentre().z - (depthA / 2);
	aMaxZ = a.GetCentre().z + (depthA / 2);

	bMinX = b.GetCentre().x - (widthB / 2);
	bMaxX = b.GetCentre().x + (widthB / 2);

	bMinY = b.GetCentre().y - (heightB / 2);
	bMaxY = b.GetCentre().y + (heightB / 2);

	bMinZ = b.GetCentre().z - (depthB / 2);
	bMaxZ = b.GetCentre().z + (depthB / 2);

	if (aMinX <= bMaxX && aMaxX >= bMinX) 
	{
		m_backwardCollision = true;
	}
	else if (aMinZ <= bMaxZ && aMaxZ >= bMinZ)
	{
		m_forwardCollision = true;
	}

	// Get two boxes and check collisions between them AABB
	return (aMinX <= bMaxX && aMaxX >= bMinX) &&
		(aMinY <= bMaxY && aMaxY >= bMinY) &&
		(aMinZ <= bMaxZ && aMaxZ >= bMinZ);
}

// Initiates player on grid
void Game::SetPlayerOnGrid() 
{
	for (int i = 0; i < m_Grid.Size(); i++)
	{
		for (int j = 0; j < m_Grid.Size(); j++)
		{
			int randomValue = rand() % (m_Grid.Size() - 1) + 1;
			// without this playercell will be at origin
			m_playerStartPosition = Vector3(randomValue, 0.5f, randomValue);
			m_playerStartRotation = Vector3(-90.0f, 0.0f, 0.0f);
			if (m_Grid.cellMatrix[i][j].GetState() == 2)
			{
				m_Grid.cellMatrix[i][j].SetPosition(m_playerStartPosition);
				m_Grid.cellMatrix[i][j].SetCentre(m_playerStartPosition);

				m_Grid.ResetPlayerInStateMatrix(j, i);

				m_Camera01.setPosition(m_Grid.cellMatrix[i][j].GetPosition());
				m_Camera01.setRotation(m_playerStartRotation);
				m_playerCells++;
				return;

			}
		}
	}
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}

#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
#ifdef DXTK_AUDIO
    m_audEngine->Suspend();
#endif
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

#ifdef DXTK_AUDIO
    m_audEngine->Resume();
#endif
}

void Game::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

#ifdef DXTK_AUDIO
void Game::NewAudioDevice()
{
    if (m_audEngine && !m_audEngine->IsAudioDevicePresent())
    {
        // Setup a retry in 1 second
        m_audioTimerAcc = 1.f;
        m_retryDefault = true;
    }
}
#endif

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
    width = 1920;
    height = 1080;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_unique<CommonStates>(device);
    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_sprites = std::make_unique<SpriteBatch>(context);
    m_font = std::make_unique<SpriteFont>(device, L"SegoeUI_18.spritefont");
	m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);
	m_postProcess = std::make_unique<BasicPostProcess>(device);

	//setup our terrain // first step for generation terrain, flat surface xz plane through 2D Array
	//m_Terrain.Initialize(device, 128, 128);
	//m_Terrain.Initialize(device, 129, 129);
	m_Terrain.Initialize(device, 257, 257); // 257

	m_Skybox.InitializeBox(device, 1.0f, 1.0f, 1.0f);

	m_cellBox.InitializeBox(device, 1, 1, 1);
	m_playerBox.InitializeBox(device, 0.5, 0.5, 0.5); // more accurate collision detection
	m_treasureBox.InitializeBox(device, 1, 1, 1);

	m_Grid.initializeGrid();

	//load and set up our Vertex and Pixel Shaders
	m_BasicShaderPair_Heightmap.InitStandard(device, L"light_vs.cso", L"light_ps.cso");
	m_BasicShaderPair_NoHeightmap.InitStandard(device, L"light_vs.cso", L"light_ps_noheightmap.cso");

	//load Textures
	CreateDDSTextureFromFile(device, L"moon.dds", nullptr, m_texture1.ReleaseAndGetAddressOf()); // terrain / floor
	CreateDDSTextureFromFile(device, L"obstaclecell.dds", nullptr, m_texture2.ReleaseAndGetAddressOf()); // obstacle cell
	CreateDDSTextureFromFile(device, L"treasurecoin.dds", nullptr, m_texture3.ReleaseAndGetAddressOf()); // treasure
	CreateDDSTextureFromFile(device, L"playercell.dds", nullptr, m_texture4.ReleaseAndGetAddressOf()); // player on minimap
	CreateDDSTextureFromFile(device, L"skybox.dds", nullptr, m_texture5.ReleaseAndGetAddressOf()); // skybox

	//Initialise Render to texture
	m_FirstRenderPass = new RenderTexture(device, 1920, 1080, 1, 2);	//last two properties are not used, cannot be zero or equal value //minimap
	m_RenderTexture = new RenderTexture(device, 1920, 1080, 1, 2); //blur
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    m_projection = Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        100.0f
    );
}

void Game::SetupGUI()
{

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Sin Wave Parameters");
		ImGui::SliderFloat("Wave Amplitude",	m_Terrain.GetAmplitude(), 0.0f, 10.0f);
		ImGui::SliderFloat("Wavelength",		m_Terrain.GetWavelength(), 0.0f, 1.0f);
	ImGui::End();
}

void Game::Blur()
{
	auto context = m_deviceResources->GetD3DDeviceContext();
	auto renderTargetView = m_deviceResources->GetRenderTargetView();
	auto depthTargetView = m_deviceResources->GetDepthStencilView();
	// Set the render target to be the render to texture.
	m_RenderTexture->setRenderTarget(context);
	// Clear the render to texture.
	m_RenderTexture->clearRenderTarget(context, 0.0f, 0.0f, 0.0f, 1.0f);

	// Draw skybox here // translate to camera position

	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthNone(), 0);
	context->RSSetState(m_states->CullNone());

	m_world = SimpleMath::Matrix::Identity;
	SimpleMath::Matrix skyboxPosition = SimpleMath::Matrix::CreateTranslation(m_Camera01.getPosition());


	m_world = m_world * skyboxPosition;
	// Turn our shaders on,  set parameters // MuseumFloor
	m_BasicShaderPair_NoHeightmap.EnableShader(context);
	m_BasicShaderPair_NoHeightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture5.Get());
	//render our model
	m_Skybox.Render(context);

	//Reset Rendering states. 
	context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
	context->RSSetState(m_states->CullClockwise());

	//prepare transform for floor object. 
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	SimpleMath::Matrix newPosition3 = SimpleMath::Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
	SimpleMath::Matrix newScale = SimpleMath::Matrix::CreateScale(0.1);		//scale the terrain down a little. 
	m_world = m_world * newScale * newPosition3;

	//setup and draw floor
	m_BasicShaderPair_Heightmap.EnableShader(context);
	m_BasicShaderPair_Heightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture1.Get());
	m_Terrain.Render(context);

	// CellGrid

	for (int i = 0; i < m_Grid.Size(); i++)
	{
		for (int j = 0; j < m_Grid.Size(); j++)
		{

			Vector3 tempPosition = DirectX::SimpleMath::Vector3(i, 0.5f, j);
			m_Grid.cellMatrix[i][j].SetPosition(tempPosition);
			m_Grid.cellMatrix[i][j].SetCentre(tempPosition);

			m_world = SimpleMath::Matrix::Identity; //set world back to identity
			newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Grid.cellMatrix[i][j].GetPosition());
			m_world = m_world * newPosition3;

			if (m_Grid.cellMatrix[i][j].GetState() == 1)
			{
				m_BasicShaderPair_NoHeightmap.EnableShader(context);
				m_BasicShaderPair_NoHeightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture2.Get());
				m_cellBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
				m_cellBox.Render(context);
			}
			else if (m_Grid.cellMatrix[i][j].GetState() == 3)
			{
				//setup and draw treasure // draw object
				m_BasicShaderPair_NoHeightmap.EnableShader(context);
				m_BasicShaderPair_NoHeightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, m_texture3.Get());
				m_treasureBox.SetCentre(m_Grid.cellMatrix[i][j].GetPosition());
				m_treasureBox.Render(context);
			}
			else {
				// do nothing
			}
		}
	}

	/* First render player so that Intersect will know beforehand where player is in current point of time */

	//prepare transform for Player object. // set coordinates
	m_world = SimpleMath::Matrix::Identity; //set world back to identity
	newPosition3 = SimpleMath::Matrix::CreateTranslation(m_Camera01.getPosition().x, m_Camera01.getPosition().y, m_Camera01.getPosition().z);
	m_playerBox.SetCentre(m_Camera01.getPosition());
	newScale = SimpleMath::Matrix::CreateScale(1);
	m_world = m_world * newScale * newPosition3;

	//setup and draw rectangle
	m_BasicShaderPair_NoHeightmap.EnableShader(context);
	m_BasicShaderPair_NoHeightmap.SetShaderParameters(context, &m_world, &m_view, &m_projection, &m_Light, NULL); // invisible collision box for player
	m_playerBox.Render(context);

	// Reset the render target back to the original back buffer and not the render to texture anymore.	
	context->OMSetRenderTargets(1, &renderTargetView, depthTargetView);

	// Apply post process effect
	m_postProcess->SetEffect(BasicPostProcess::GaussianBlur_5x5);
	m_postProcess->SetSourceTexture(m_RenderTexture->getShaderResourceView());
	m_postProcess->Process(context);
}

void Game::OnDeviceLost()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_font.reset();
	m_batch.reset();
	m_testmodel.reset();
    m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion
