/*
** Copyright (c) 2018 LunarG, Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#include <cassert>
#include <cstdlib>

#include "volk.h"

#include "application/xcb_window.h"

BRIMSTONE_BEGIN_NAMESPACE(brimstone)
BRIMSTONE_BEGIN_NAMESPACE(application)

XcbWindow::XcbWindow(XcbApplication* application) :
    xcb_application_(application), xpos_(0), ypos_(0), width_(0), height_(0), window_(0),
    atom_wm_delete_window_(nullptr)
{
    assert(application != nullptr);
}

XcbWindow::~XcbWindow()
{
    if (window_ != 0)
    {
        xcb_destroy_window(xcb_application_->GetConnection(), window_);
    }
}

bool XcbWindow::Create(const int32_t x, const int32_t y, const uint32_t width, const uint32_t height)
{
    xcb_connection_t* connection = xcb_application_->GetConnection();
    xcb_screen_t*     screen     = xcb_application_->GetScreen();

    window_ = xcb_generate_id(connection);
    xpos_   = x;
    ypos_   = y;
    width_  = width;
    height_ = height;

    xcb_application_->RegisterXcbWindow(this);

    uint32_t value_mask   = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t value_list[] = { screen->black_pixel,
                              XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY };

    xcb_create_window(connection,
                      XCB_COPY_FROM_PARENT,
                      window_,
                      screen->root,
                      static_cast<int16_t>(x),
                      static_cast<int16_t>(y),
                      static_cast<uint16_t>(width),
                      static_cast<uint16_t>(height),
                      0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual,
                      value_mask,
                      value_list);

    // Magic code that will send notification when window is destroyed.
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply  = xcb_intern_atom_reply(connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
    atom_wm_delete_window_           = xcb_intern_atom_reply(connection, cookie2, 0);

    xcb_change_property(
        connection, XCB_PROP_MODE_REPLACE, window_, reply->atom, 4, 32, 1, &(atom_wm_delete_window_->atom));
    free(reply);

    // Make sure window is visible.
    xcb_map_window(connection, window_);
    xcb_flush(connection);

    return true;
}

bool XcbWindow::Destroy()
{
    if (window_ != 0)
    {
        xcb_destroy_window(xcb_application_->GetConnection(), window_);
        xcb_application_->UnregisterXcbWindow(this);
        window_ = 0;
        return true;
    }

    return false;
}

void XcbWindow::SetPosition(const int32_t x, const int32_t y)
{
    if (x != xpos_ || y != ypos_)
    {
        xpos_ = x;
        ypos_ = y;

        xcb_connection_t* connection = xcb_application_->GetConnection();
        uint32_t          values[]   = { static_cast<uint32_t>(x), static_cast<uint32_t>(y) };

        xcb_configure_window(connection, window_, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
        xcb_flush(connection);
    }
}

void XcbWindow::SetSize(const uint32_t width, const uint32_t height)
{
    if (width != width_ || height != height_)
    {
        width_  = width;
        height_ = height;

        xcb_connection_t* connection = xcb_application_->GetConnection();
        uint32_t          values[]   = { width, height };

        xcb_configure_window(connection, window_, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
        xcb_flush(connection);
    }
}

void XcbWindow::SetVisibility(bool show)
{
    xcb_connection_t* connection = xcb_application_->GetConnection();

    if (show)
    {
        xcb_map_window(connection, window_);
    }
    else
    {
        xcb_unmap_window(connection, window_);
    }

    xcb_flush(connection);
}

void XcbWindow::SetForeground()
{
    xcb_connection_t* connection = xcb_application_->GetConnection();
    uint32_t          values[]   = { XCB_STACK_MODE_ABOVE };

    xcb_configure_window(connection, window_, XCB_CONFIG_WINDOW_STACK_MODE, values);
    xcb_flush(connection);
}

bool XcbWindow::GetNativeHandle(uint32_t id, void** handle)
{
    assert(handle != nullptr);
    switch (id)
    {
        case XcbWindow::kConnection:
            *handle = reinterpret_cast<void*>(xcb_application_->GetConnection());
            return true;
        case XcbWindow::kWindow:
            *handle = reinterpret_cast<void*>(window_);
            return true;
        default:
            return false;
    }
}

VkResult XcbWindow::CreateSurface(VkInstance instance, VkFlags flags, VkSurfaceKHR* pSurface)
{
    VkXcbSurfaceCreateInfoKHR create_info{
        VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR, nullptr, flags, xcb_application_->GetConnection(), window_
    };

    return vkCreateXcbSurfaceKHR(instance, &create_info, nullptr, pSurface);
}

XcbWindowFactory::XcbWindowFactory(XcbApplication* application) : xcb_application_(application)
{
    assert(application != nullptr);
}

format::Window* XcbWindowFactory::Create(const int32_t x, const int32_t y, const uint32_t width, const uint32_t height)
{
    auto window = new XcbWindow(xcb_application_);
    window->Create(x, y, width, height);
    return window;
}

VkBool32 XcbWindowFactory::GetPhysicalDevicePresentationSupport(VkPhysicalDevice physical_device,
                                                                uint32_t         queue_family_index)
{
    xcb_connection_t* connection = xcb_application_->GetConnection();
    xcb_screen_t*     screen     = xcb_application_->GetScreen();

    assert((connection != nullptr) && (screen != nullptr));

    return vkGetPhysicalDeviceXcbPresentationSupportKHR(
        physical_device, queue_family_index, connection, screen->root_visual);
}

BRIMSTONE_END_NAMESPACE(application)
BRIMSTONE_END_NAMESPACE(brimstone)
