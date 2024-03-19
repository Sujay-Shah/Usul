#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "Engine/Core/EngineDefines.h"

#include "Engine/Core/EngineApp.h"

#include "Engine/Core/Input.h"
#include "Engine/Core/Logging.h"
#include "Engine/Core/Assert.h"

#include "Engine/Event/Event.h"
#include "Engine/Event/KeyCodes.h"
#include "Engine/Event/MouseCodes.h"

#include "Engine/Core/Layer.h"
#include "Engine/Core/LayerStack.h"
#include "Engine/ImGui/ImGuiLayer.h"



#include "Engine/Core/Timestep.h"

//Renderer
#include "Renderer/Renderer.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/VertexArray.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"
#include "Renderer/Camera/CameraController.h"
#include "Renderer/FrameBuffer.h"
#include "Renderer/Material.h"
#include "Renderer/Model.h"

#endif