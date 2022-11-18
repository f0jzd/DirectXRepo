//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include <ReadData.h>


extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;


struct VS_CONSTANT_BUFFER
{
    alignas(16) float time;

}VsConstData;




Game::Game() noexcept(false) : m_fullscreenRect{}, m_bloomRect{}
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);

    const auto format = m_deviceResources->GetBackBufferFormat();
    m_offscreenTexture = std::make_unique<DX::RenderTexture>(format);
    m_renderTarget1 = std::make_unique<DX::RenderTexture>(format);
    m_renderTarget2 = std::make_unique<DX::RenderTexture>(format);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    
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
    auto totalTime = static_cast<float>(timer.GetTotalSeconds());

    auto context = m_deviceResources->GetD3DDeviceContext();
     
    m_world = Matrix::CreateRotationZ(totalTime / 2.f)
        * Matrix::CreateRotationY(totalTime)
        * Matrix::CreateRotationX(totalTime * 2.f);

    
    VsConstData.time += elapsedTime;

    context->UpdateSubresource(m_chromAbberationParam.Get(), 0, nullptr, &VsConstData, sizeof(VS_CONSTANT_BUFFER), 0);

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
    m_spriteBatch->Draw(m_background.Get(), m_fullscreenRect);
    m_spriteBatch->End();

    m_shape->Draw(m_world, m_view, m_projection);

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    PostProcess();
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_offscreenTexture->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

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
    auto context = m_deviceResources->GetD3DDeviceContext();


    // TODO: Initialize device dependent objects here (independent of window size).

    context = m_deviceResources->GetD3DDeviceContext();

    DX::ThrowIfFailed(CreateWICTextureFromFile(device,
        L"sunset.jpg", nullptr,
        m_background.ReleaseAndGetAddressOf()));

    auto blob = DX::ReadData(L"ChromaticAbberation.cso");
    DX::ThrowIfFailed(device->CreatePixelShader(blob.data(), blob.size(),
        nullptr, m_chromAbberation.ReleaseAndGetAddressOf()));

    CD3D11_BUFFER_DESC cbDesc(sizeof(VS_CONSTANT_BUFFER),
        D3D11_BIND_CONSTANT_BUFFER);

   

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = &VsConstData;
    InitData.SysMemPitch = 0;
    InitData.SysMemSlicePitch = 0;

    device->CreateBuffer(&cbDesc, &InitData, m_chromAbberationParam.GetAddressOf());        

    m_offscreenTexture->SetDevice(device);
    m_renderTarget1->SetDevice(device);
    m_renderTarget2->SetDevice(device);

    m_states = std::make_unique<CommonStates>(device);

    m_spriteBatch = std::make_unique<SpriteBatch>(context);
    m_shape = GeometricPrimitive::CreateTorus(context);

    m_view = Matrix::CreateLookAt(Vector3(0.f, 3.f, -3.f),
        Vector3::Zero, Vector3::UnitY);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.

    auto context = m_deviceResources->GetD3DDeviceContext();
    auto size = m_deviceResources->GetOutputSize();
    m_fullscreenRect = size;
    m_projection = Matrix::CreatePerspectiveFieldOfView(XM_PIDIV4,
        float(size.right) / float(size.bottom), 0.01f, 100.f);

    m_offscreenTexture->SetWindow(size);

    // Half-size blurring render targets
    m_bloomRect = { 0, 0, size.right / 2, size.bottom / 2 };

    m_renderTarget1->SetWindow(m_bloomRect);
    m_renderTarget2->SetWindow(m_bloomRect);

    context->UpdateSubresource(m_chromAbberationParam.Get(), 0, nullptr, &VsConstData, sizeof(VS_CONSTANT_BUFFER), 0);
}


void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_states.reset();
    m_spriteBatch.reset();
    m_shape.reset();
    m_background.Reset();
    m_offscreenTexture->ReleaseDevice();
    m_renderTarget1->ReleaseDevice();
    m_renderTarget2->ReleaseDevice();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
void Game::PostProcess()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    ID3D11ShaderResourceView* null[] = { nullptr, nullptr };

    context->CopyResource(m_deviceResources->GetRenderTarget(),
        m_offscreenTexture->GetRenderTarget());

    auto renderTarget = m_deviceResources->GetRenderTargetView();
    context->OMSetRenderTargets(1, &renderTarget, nullptr);
    auto rtSRV = m_offscreenTexture->GetShaderResourceView();

    m_spriteBatch->Begin(SpriteSortMode_Immediate,
        nullptr, nullptr, nullptr, nullptr,
        [=]() {
            context->PSSetShader(m_chromAbberation.Get(), nullptr, 0);
            context->PSSetConstantBuffers(0, 1, m_chromAbberationParam.GetAddressOf());
        });
    m_spriteBatch->Draw(rtSRV, m_fullscreenRect);
    m_spriteBatch->End();

    context->PSSetShaderResources(0, 2, null);
}

#pragma endregion