//
// Game.cpp -
//

#include "pch.h"
#include "Game.h"

using namespace DirectX;

using Microsoft::WRL::ComPtr;

// Constructor.
Game::Game()
{
    m_deviceResources.reset( new DX::DeviceResources );
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window)
{
    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDevice();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
}

// Executes basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here
    elapsedTime;
}

// Draws the scene
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
        return;

    Clear();

    // TODO: Add your rendering code here

    m_deviceResources->Present();
}

// Helper method to clear the backbuffers
void Game::Clear()
{
    // Clear the views
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetBackBufferRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);
}

// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized)
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize)
}

void Game::OnWindowSizeChanged()
{
    if (!m_deviceResources->WindowSizeChanged())
        return;

    CreateResources();

    // TODO: Game window is being resized
}

// Properties
void Game::GetDefaultSize(size_t& width, size_t& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200)
    width = 800;
    height = 600;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    // TODO: Initialize device dependent objects here (independent of window size)
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // TODO: Initialize windows-size dependent objects here
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here
}

void Game::OnDeviceRestored()
{
    CreateDevice();

    CreateResources();
}