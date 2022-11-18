//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include <algorithm>


extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    // TODO: Provide parameters for swapchain format, depth/stencil format, and backbuffer count.
    //   Add DX::DeviceResources::c_AllowTearing to opt-in to variable rate displays.
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);


    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    auto kb = m_keyboard->GetState();
    m_keys.Update(kb);


    if (kb.Escape)
    {
        ExitGame();
    }
    auto mouse = m_mouse->GetState();
    m_mouseButtons.Update(mouse);

    Direction = Vector2::Zero;
        

    if (m_keys.pressed.W && !isAnimating)
    {
        Direction.y = -100;

        startPos = m_screenPos;
        targetPos = startPos + Direction;
        isAnimating = true;
        animTime = 0;

    }
    if (m_keys.pressed.S && !isAnimating)
    {
        Direction.y = 100;

        startPos = m_screenPos;
        targetPos = startPos + Direction;
        isAnimating = true;
        animTime = 0;

    } 
    if (m_keys.pressed.A && !isAnimating)
    {
        Direction.x = -100;

        startPos = m_screenPos;
        targetPos = startPos + Direction;
        isAnimating = true;
        animTime = 0;
       
    }    
    if (m_keys.pressed.D && !isAnimating)
    {
        Direction.x = 100;

        startPos = m_screenPos;
        targetPos = startPos + Direction;
        isAnimating = true;
        animTime = 0;
    }

    if (isAnimating)
    {
        animTime += elapsedTime;
        float t = std::clamp<float>((animTime / moveTime),0,1);


        auto t_lam = [=]{
            return 1.0f - QuadEaseIn(1 - t);
        };

        t = t_lam();

        //t = QuadEaseOut(t);

        m_screenPos =  Vector2::Lerp(startPos, targetPos, t);

        if (t >= 1.0f)
            isAnimating = false;
       
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

    // TODO: Add your rendering code here.
    m_spriteBatch->Begin();

    m_spriteBatch->Draw(m_texture.Get(), m_screenPos, nullptr, Colors::White, 0.f,m_origin);
    
    m_spriteBatch->End();
    context;

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
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
    auto const viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
    m_keys.Reset();
    m_mouseButtons.Reset();
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();
    m_keys.Reset();
    m_mouseButtons.Reset();
    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    // TODO: Initialize device dependent objects here (independent of window size).

    auto context = m_deviceResources->GetD3DDeviceContext();
    m_spriteBatch = std::make_unique<SpriteBatch>(context);

    ComPtr<ID3D11Resource> resource;

    DX::ThrowIfFailed(CreateWICTextureFromFile(device, L"cat.png", resource.GetAddressOf(), m_texture.ReleaseAndGetAddressOf()));

    ComPtr<ID3D11Texture2D> cat;
    DX::ThrowIfFailed(resource.As(&cat));

    CD3D11_TEXTURE2D_DESC catDesc;
    cat->GetDesc(&catDesc);

    m_origin.x = float(catDesc.Width / 2);
    m_origin.y = float(catDesc.Height / 2);


    device;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    auto size = m_deviceResources->GetOutputSize();
    m_screenPos.x = float(size.right) / 2.f;
    m_screenPos.y = float(size.bottom) / 2.f;

}

float Game::QuadEaseIn(float t)
{
    return t*t;
}

float Game::QuadEaseOut(float t)
{
    return 1.0f - QuadEaseIn(1 - t);
}


void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.

    m_texture.Reset();
    m_spriteBatch.reset();

}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}




#pragma endregion
