#include "SoundPlayer.hpp"

#include <Engine/Audio/SoundHandler.hpp>
#include <Engine/Rendering/Components/ComponentGroups.hpp>
#include <Engine/Rendering/Components/VPMatrices.hpp>
#include <Engine/Physics/Velocity.hpp>
#include <Engine/Transform.hpp>

SoundPlayer::SoundPlayer(ECSCore* pECS)
    :System(pECS),
    m_pLightHandler(nullptr),
    m_pSoundHandler(nullptr),
    m_pTransformHandler(nullptr),
    m_pVelocityHandler(nullptr)
{
    CameraComponents cameraComponents;
    SystemRegistration sysReg = {};
    sysReg.pSystem = this;
    sysReg.SubscriberRegistration.ComponentSubscriptionRequests = {
        {{{RW, g_TIDSound}, {R, g_TIDPosition}}, &m_Sounds},
        {{{R, g_TIDVelocity}}, {&cameraComponents}, &m_Cameras}
    };

    subscribeToComponents(sysReg);
    registerUpdate(sysReg);
}

SoundPlayer::~SoundPlayer()
{}

bool SoundPlayer::initSystem()
{
    m_pSoundHandler     = reinterpret_cast<SoundHandler*>(getComponentHandler(TID(SoundHandler)));
    m_pTransformHandler = reinterpret_cast<TransformHandler*>(getComponentHandler(TID(TransformHandler)));
    m_pLightHandler     = reinterpret_cast<LightHandler*>(getComponentHandler(TID(LightHandler)));
    m_pVelocityHandler  = reinterpret_cast<VelocityHandler*>(getComponentHandler(TID(VelocityHandler)));

    return m_pSoundHandler && m_pTransformHandler && m_pLightHandler && m_pVelocityHandler;
}

void SoundPlayer::update([[maybe_unused]] float dt)
{
    FMOD::System* pSystem = m_pSoundHandler->getSystem();
    pSystem->update();

    if (m_Cameras.empty()) {
        return;
    }

    const DirectX::XMFLOAT3& cameraPosition = m_pTransformHandler->getPosition(m_Cameras[0]);
    const DirectX::XMFLOAT4& camRotationQuaternion = m_pTransformHandler->getRotation(m_Cameras[0]);

    DirectX::XMVECTOR camPos = DirectX::XMLoadFloat3(&cameraPosition);
    DirectX::XMVECTOR camDir = m_pTransformHandler->getForward(camRotationQuaternion);
    DirectX::XMVECTOR camDirFlat = camDir;
    camDirFlat = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(camDirFlat, 0.0f));

    const DirectX::XMVECTOR camVelocity = DirectX::XMLoadFloat3(&m_pVelocityHandler->getVelocity(m_Cameras[0]));
    float camSpeed = DirectX::XMVectorGetX(DirectX::XMVector3Length(camVelocity));

    size_t soundCount = m_Sounds.size();
    IDDVector<Sound>& sounds = m_pSoundHandler->m_Sounds;

    for (Entity soundEntity : m_Sounds.getIDs()) {
        Sound& sound = sounds.indexID(soundEntity);
        DirectX::XMFLOAT3& soundPosition = m_pTransformHandler->getPosition(soundEntity);

        DirectX::XMVECTOR soundPos = DirectX::XMLoadFloat3(&soundPosition);

        // Calculate volume using distance to camera
        DirectX::XMVECTOR camToSound = DirectX::XMVectorSubtract(soundPos, camPos);
        float soundDistance = DirectX::XMVectorGetX(DirectX::XMVector3Length(camToSound));
        float volume = 1.0f / soundDistance;

        sound.pChannel->setVolume(volume);

        stereoPan(sound, camToSound, camDirFlat);

        DirectX::XMFLOAT3 objectVelocity = {0.0f, 0.0f, 0.0f};
        if (m_pVelocityHandler->hasVelocity(soundEntity)) {
            objectVelocity = m_pVelocityHandler->getVelocity(soundEntity);
        }

        dopplerEffect(sound, camPos, camVelocity, camSpeed, soundPos, objectVelocity);
    }
}

void SoundPlayer::stereoPan(Sound& sound, const DirectX::XMVECTOR& camToSound, const DirectX::XMVECTOR& camDirFlat)
{
    // Ignore vertical difference between camera and sound, this allows for calculating the yaw
    DirectX::XMVECTOR camToSoundNorm = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(camToSound, 0.0f));

    float angle = -m_pTransformHandler->getOrientedAngle(camToSound, camDirFlat, g_DefaultUp);
    float cosAngle = std::cosf(angle);
    float sinAngle = std::sinf(angle);

    float sqrtTwoRec = 1.0f / std::sqrtf(2.0f);
    float ampLeft   = 0.5f * (sqrtTwoRec * (cosAngle + sinAngle)) + 0.5f;
    float ampRight  = 0.5f * (sqrtTwoRec * (cosAngle - sinAngle)) + 0.5f;

    sound.pChannel->setMixLevelsOutput(ampLeft, ampRight, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

void SoundPlayer::dopplerEffect(Sound& sound, const DirectX::XMVECTOR& camPos, const DirectX::XMVECTOR& camVelocity, float camSpeed, const DirectX::XMVECTOR& objectPos, const DirectX::XMFLOAT3& objectVelocity)
{
    DirectX::XMVECTOR objVelocity = DirectX::XMLoadFloat3(&objectVelocity);

    // Calculate signs for velocities
    DirectX::XMVECTOR camToObject = DirectX::XMVectorSubtract(objectPos, camPos);

    // camVelocity is positive if it is moving towards the sound
    float camVelocitySign = DirectX::XMVectorGetX(DirectX::XMVector3Dot(camVelocity, camToObject));

    // objectVelocity is positive if it is moving away from the receiver
    float objectVelocitySign = DirectX::XMVectorGetX(DirectX::XMVector3Dot(objVelocity, camToObject));

    camSpeed = camVelocitySign < 0.0f ? -camSpeed : camSpeed;
    float objSpeed = DirectX::XMVectorGetX(DirectX::XMVector3Length(objVelocity));
    objSpeed = objectVelocitySign < 0.0f ? -objSpeed : objSpeed;

    const float soundPropagation = 343.0f;
    float frequency = 0.0f;
    sound.pChannel->getFrequency(&frequency);

    float observedFrequency = frequency * (soundPropagation + camSpeed) / (soundPropagation + objSpeed);

    sound.pChannel->setFrequency(observedFrequency);
}
