//
// Created by snsha on 2024-02-13.
//

#include "VulkanExample.h"

VulkanExample::~VulkanExample() {

}

void VulkanExample::OnUpdate(const Engine::Timestep &ts) {
    Layer::OnUpdate(ts);
}

void VulkanExample::OnAttach() {
    Layer::OnAttach();
}

void VulkanExample::OnDetach() {
    Layer::OnDetach();
}

void VulkanExample::OnEvent(Engine::Event &e) {
    Layer::OnEvent(e);
}

void VulkanExample::OnImGuiRender() {
    Layer::OnImGuiRender();
}

VulkanExample::VulkanExample()
: Layer("VulkanExample")
{


}
