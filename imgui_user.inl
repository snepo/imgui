//
//  imgui_user.h
//  Snepo
//
//  Created by Andrew Wright on 19/09/2016.
//
//

#include "CinderImGui.h"

using namespace std;
using namespace ci;

#if __has_include(<Snepo/Lucy.h>)

#include <Snepo/Lucy/UI/Actor.h>
#include <Snepo/Lucy/Core.h>

using namespace Lucy;
using namespace Lucy::Core;

#define UI_NAME() ( ( actor->GetName() + "##" + actor->GetName() + "_" + std::to_string(actor->UID()) ).c_str() )
#define UI_PROP(p) ( ( Lucy::String(p) + "##" + p + "_" + std::to_string(actor->UID()) ).c_str() )

void ImGui::InstallLogger ( )
{
    static bool installed = false;
    if ( !installed )
    {
        Log::Logger::GetLogger()->InstallDefaultEngine();
        Log::Logger::GetLogger()->AddLoggingEngine ( Factory::Create<Log::CyclingLoggingEngine>( 128 ) );

        installed = true;
    }
}

void ImGui::Logger ( )
{
    InstallLogger();
    
    static std::unordered_map<Log::Level, ImVec4> kColorMap =
    {
        { Log::Level::Info,    ImVec4(0, 1.0f, 0, 1.0f) },
        { Log::Level::Debug,   ImVec4(1.0f, 1.0f, 1.0f, 1.0f) },
        { Log::Level::Warning, ImVec4(1.0f, 1.0f, 0, 1.0f) },
        { Log::Level::Error,   ImVec4(1.0f, 0.5f, 0, 1.0f) },
        { Log::Level::Fatal,   ImVec4(1.0f, 0, 0, 1.0f) }
    };
    
    ui::ScopedWindow window ( "Log" );
    
    auto cycle = static_cast<Log::CyclingLoggingEngine *> ( Log::Logger::GetLogger()->GetLoggingEngines().back().get() );
    for ( auto& m : cycle->Queue() )
    {
        if ( m.GetMessage().empty() ) continue;
        ui::TextColored( kColorMap[m.GetLevel()], "%s", m.GetMessage().c_str() );
    }
    
    ImGui::SetScrollHere(1.0f);
}

void InspectActor ( const Lucy::UI::ActorRef& actor )
{
    bool tree = ui::TreeNode ( UI_NAME() );
    
    if ( ui::GetIO().KeyShift && ui::IsItemHovered() )
    {
        auto screen = actor->GetScreenBounds();
        
        auto ul = ImVec2(screen.getUpperLeft().x, screen.getUpperLeft().y);
        auto lr = ImVec2(screen.getLowerRight().x, screen.getLowerRight().y);

        ImGuiState& g = *GImGui;

        ui::SetNextWindowPos ( ImVec2 ( 0.0f, 0.0f ) );
        ui::ScopedStyleColor color ( ImGuiCol_WindowBg, ImVec4 ( 0, 0, 0, 0 ) );
        ui::ScopedWindow debug ( "##DebugStrokeLayer", ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoSavedSettings );
        {
            g.OverlayDrawList.AddRectFilled ( ul, lr, ( actor->UID() | 0x11000000 ) );
            g.OverlayDrawList.AddText ( ul, 0xFFFFFFFF, actor->GetName().c_str() );
        }
    }

    if ( tree )
    {
        if ( ui::DragFloat2(UI_PROP("Position"), &actor->PositionAnim()().x ) );
        if ( ui::DragFloat2(UI_PROP("Scale"), &actor->ScaleAnim()().x, 0.0f, -5.0f, 5.0f ) );
        if ( ui::DragFloat(UI_PROP("Rotation"), &actor->RotationAnim()(), 0.0f, -M_PI, M_PI ) );
        bool v = actor->IsVisible();
        if ( ui::Checkbox(UI_PROP("Visible"), &v ) ) actor->SetVisible(v);
        
        for ( auto& c : actor->GetChildren() )
        {
            if ( c->Inspectable() )
            {
                InspectActor( c );
            }
        }
        ui::TreePop();
    }
}

void ImGui::SceneGraph ( )
{ 
    InspectActor( Lucy::UI::Stage::GetStage() );
}

#endif

bool ImGui::ColorPicker ( const char * label, float col[3] )
{
    vec3 hsv;
    ImVec4 rgba( col[0], col[1], col[2], 1.0f );

    ColorConvertRGBtoHSV( col[0], col[1], col[2], hsv.x, hsv.y, hsv.z );
    
    auto popupLabel = std::string("ColorPickerPopup##") + label;
    
    if( ColorButton( rgba ) )
    {
        OpenPopup( popupLabel.c_str() );
    }
    
    if ( label[0] != '#' )
    {
        SameLine();
        Text( "%s", label );
    }
    
    vec2 texSize( 128 );
    bool changes = false;
    static bool needTexUpdate = true;
    static gl::Texture2dRef svTex, hueTex;
    
    if( needTexUpdate )
    {
        Surface svSurface( texSize.x, texSize.y, false );
        for( float x = 0; x < texSize.x; ++x )
        {
            for( float y = 0; y < texSize.y; ++y )
            {
                ColorA color( ColorModel::CM_HSV, hsv.x, x / texSize.x, 1.0f - y / texSize.y );
                svSurface.setPixel( ivec2( x, y ), color );
            }
        }

        svTex = gl::Texture2d::create( svSurface );
        
        needTexUpdate = false;
    }

    if( !hueTex )
    {
        Surface hueSurface( 10, texSize.y, false );
        for( float x = 0; x < 10; ++x )
        {
            for( float y = 0; y < texSize.y; ++y )
            {
                ColorA color( ColorModel::CM_HSV, y / texSize.y, 1.0f, 1.0f );
                hueSurface.setPixel( ivec2( x, y ), color );
            }
        }
        hueTex = gl::Texture2d::create( hueSurface );
    }
    
    SetNextWindowSize( ImVec2( texSize.y + 34, texSize.y + 14 ) );
    
    if( BeginPopupContextItem( popupLabel.c_str() ) )
    {
        Image( svTex, ImVec2( texSize.x, texSize.y ) );
        
        ImGuiWindow* window = GetCurrentWindow();
        window->DrawList->AddCircle( GetItemBoxMin() + ImVec2( texSize.x * hsv.y, texSize.y * ( 1.0f - hsv.z ) ), 3, 0xffffffff );
        if( IsItemHovered() && IsMouseDown( 0 ) )
        {
            hsv.y           = (float) ( GetMousePos().x - GetItemRectMin().x ) / (float) GetItemRectSize().x;
            hsv.z           = 1.0f - (float) ( GetMousePos().y - GetItemRectMin().y ) / (float) GetItemRectSize().y;
            needTexUpdate   = true;
            changes         = true;
        }

        SameLine();
        Image( hueTex, ImVec2( 10, texSize.y ) );
        
        float hueY = hsv.x * (float) GetItemRectSize().y + GetItemBoxMin().y;
        window->DrawList->AddTriangleFilled( ImVec2( GetItemBoxMin().x - 3, hueY - 3 ), ImVec2( GetItemBoxMin().x - 3, hueY + 3 ), ImVec2( GetItemBoxMin().x + 1, hueY ), 0xffffffff );
        window->DrawList->AddTriangleFilled( ImVec2( GetItemBoxMax().x + 3, hueY - 3 ), ImVec2( GetItemBoxMax().x + 3, hueY + 3 ), ImVec2( GetItemBoxMax().x - 1, hueY ), 0xffffffff );
        if( IsItemHovered() && IsMouseDown( 0 ) )
        {
            hsv.x   = (float) ( GetMousePos().y - GetItemRectMin().y ) / (float) GetItemRectSize().y;
            changes = true;
            needTexUpdate = true;
        }
        
        hsv = glm::clamp( hsv, vec3(0), vec3(1) );
        
        ColorA converted = ColorA( ColorModel::CM_HSV, hsv.x, hsv.y, hsv.z );
        col[0] = converted.r;
        col[1] = converted.g;
        col[2] = converted.b;
        
        EndPopup();
    }
    
    return changes;
}

static struct TimelineGlobals
{
    float * kMaxTimelineValue{nullptr};
    float * kCurrentTimelineValue{nullptr};

    float   kMaxTimelineValueCached{0};
    ImVec2  kBaseCursorPos;

    bool    kDidDragItemThisFrame{false};
} kTimelineGlobals; // Make it a std::map<String, TimelineGlobals> to support multiple instances

const float kHandleRadius = 6.0f;
const float kHandleSize   = 2 * kHandleRadius;
const float kRowHeight    = 19.0f;

namespace ImGui
{
    float Snap ( float value, float period, float min = 0.0f, float max = kMaxTimelineValueCached )
    {
        float halfPeriod = period * 0.5f;
        return glm::clamp ( ( value ) - ( fmodf ( value + halfPeriod, period ) ), min - halfPeriod, max - halfPeriod ) + halfPeriod;
    }
    
    void BeginTimeline ( const char* str_id, float * currentTime, float * totalTime )
    {
        kTimelineGlobals.kMaxTimelineValue          = totalTime;
        kTimelineGlobals.kCurrentTimelineValue      = currentTime;
        kTimelineGlobals.kDidDragItemThisFrame      = false;
        
        kTimelineGlobals.kMaxTimelineValueCached    = *totalTime;
        kTimelineGlobals.kBaseCursorPos             = GetCursorPos();

        return true;
    }
    
    bool TimelineEvent ( const char * str_id, float * values )
    {
        ui::ScopedStyleVar spacing ( ImGuiStyleVar_ItemSpacing, ImVec2 ( 6, 6 ) );
        ui::ScopedStyleVar padding ( ImGuiStyleVar_WindowPadding, ImVec2 ( 0, 0 ) );
        
        const ImU32 inactiveColor = ColorConvertFloat4ToU32 ( GetStyle().Colors[ImGuiCol_Button] );
        const ImU32 activeColor   = ColorConvertFloat4ToU32 ( GetStyle().Colors[ImGuiCol_ButtonHovered] );
        const ImU32 lineColor     = ColorConvertFloat4ToU32 ( GetStyle().Colors[ImGuiCol_ColumnActive] );
        
        auto draw                 = GetWindowDrawList();

        bool changed              = false;
        ImVec2 startCursorPos     = GetCursorPos();
        
        float totalLeft           = GetCursorPos().x + kHandleRadius;
        float totalRight          = GetWindowWidth() - 3 * kHandleRadius;
        
        float totalWidth          = totalRight - totalLeft;

        float lowRatio            = values[0] / kTimelineGlobals.kMaxTimelineValueCached;
        float highRatio           = values[1] / kTimelineGlobals.kMaxTimelineValueCached;
        
        float x0                  = lerp ( totalLeft, totalRight, lowRatio  ) - kHandleRadius;
        float x1                  = lerp ( totalLeft, totalRight, highRatio ) - kHandleRadius;
        
        float y                   = GetCursorPos().y + 2.0f;
        
        ImVec2 screenOffset       = GetWindowPos() + ImVec2(0, -GetScrollY());
        
        auto updateValues         = [&] ( float * values, uint32_t numValues )
        {
            if ( IsItemActive() && IsMouseDragging(0))
            {
                float delta = GetIO().MouseDelta.x / totalWidth * kTimelineGlobals.kMaxTimelineValueCached;
                
                for ( uint32_t i = 0; i < numValues; i++ )
                {
                    values[i] += delta;
                }
                
                changed = true;
                kTimelineGlobals.kDidDragItemThisFrame = true;
            }
        };
        
        auto doHandle             = [&] ( uint32_t index, const ImVec2& position )
        {
            ui::SetCursorPos( position );
            ui::ScopedId id( index );
            ui::InvisibleButton(str_id, ImVec2(kHandleSize, kHandleSize) );
            {
                auto localPos   = ImVec2(position.x + kHandleRadius, position.y + kHandleRadius);
                auto screenPos  = screenOffset + localPos;
                bool active     = IsItemActive() || IsItemHovered();
                
                draw->AddCircleFilled( screenPos, kHandleRadius, active ? activeColor : inactiveColor );
                updateValues ( &values[index], 1 );
                
                if ( active )
                {
                    ImVec2 a = GetWindowPos() + ImVec2(localPos.x, 0);
                    ImVec2 b = GetWindowPos() + ImVec2(localPos.x, GetWindowHeight());
                    
                    ImGui::SetTooltip("%.2f", values[index]);
                    draw->AddLine( a, b, lineColor);
                }
            }
        };

        doHandle ( 0, ImVec2 ( x0, y ) );
        doHandle ( 1, ImVec2 ( x1, y ) );
        
        {
            ui::ScopedId id(2);
            ui::SetCursorPos( ImVec2 ( x0 + kHandleSize, y + kHandleRadius / 2.0f ) );
            
            ImVec2 size ( x1 - ( x0 + kHandleSize ), kHandleRadius );
            ImVec2 UL ( x0 + kHandleSize, y + kHandleRadius / 2.0f );
            ImVec2 LR ( UL + size );
            
            ui::InvisibleButton( str_id, size );
            {
                draw->AddRectFilled( screenOffset + UL, screenOffset + LR, IsItemActive() || IsItemHovered() ? activeColor : inactiveColor );
                updateValues ( &values[0], 2 );
            }
        }
        
        if ( changed )
        {
            if ( values[1] < values[0] )
            {
                std::swap ( values[0], values[1] );
            }
            
            if ( values[0] < 0.0f ) values[0] = 0.0f;
            if ( values[1] > kTimelineGlobals.kMaxTimelineValueCached ) values[1] = kTimelineGlobals.kMaxTimelineValueCached;
        }
        
        SetCursorPos ( ImVec2 ( startCursorPos.x, startCursorPos.y + kRowHeight ) );
       
        return changed;
    }
    
    void EndTimeline ( )
    {
        SetCursorPos( kTimelineGlobals.kBaseCursorPos );
        
        const ImVec2 textOffset { -5, GetTextLineHeightWithSpacing() * 3.0f };

        auto draw             = GetWindowDrawList();

        ImU32 activeLineColor = ColorConvertFloat4ToU32 ( GetStyle().Colors[ImGuiCol_ColumnActive] );
        ImU32 lineColor       = 0x08FFFFFF;
        ImU32 textColor       = 0x33FFFFFF;
        
        float totalLeft       = GetCursorPos().x + kHandleRadius;
        float totalRight      = GetWindowWidth() - 3 * kHandleRadius;
        
        float x0              = lerp ( totalLeft, totalRight, 0.0f );
        float x1              = lerp ( totalLeft, totalRight, 1.0f );
        
        float y0              = GetCursorPos().y;
        float y1              = GetCursorPos().y + GetWindowHeight();
        
        float totalWidth      = x1 - x0;
        ImVec2 localOffset    = GetWindowPos();
        
        uint32_t numLines     = std::max ( static_cast<int> ( x1 - x0 ) / 150, 2 );
        
        float xStep           = totalWidth / static_cast<float>(numLines);
        float timeStep        = kTimelineGlobals.kMaxTimelineValueCached / static_cast<float>(numLines);
        
        for ( int i = 0; i <= numLines; i++ )
        {
            char tmp[64];
            ImFormatString(tmp, sizeof(tmp), "%.2f", i * timeStep );
            
            ImVec2 a = localOffset + ImVec2 ( x0 + i * xStep, y0 );
            ImVec2 b = localOffset + ImVec2 ( x0 + i * xStep, y1 );
            
            ImVec2 extraTextOffset{0, 0};
            if ( i == numLines )
            {
                extraTextOffset.x = -CalcTextSize(tmp).x - 10;
            }
            
            draw->AddLine ( a, b, lineColor );
            draw->AddText ( b - textOffset + extraTextOffset, textColor, tmp);
        }
        
        if ( !kTimelineGlobals.kDidDragItemThisFrame && IsMouseHoveringWindow() && IsMouseDragging(0) )
        {
            float r = glm::clamp( ( ( GetMousePos().x ) - x0 ) / ( x1 - x0 ), 0.0f, 1.0f );
            float newTime = r * *kTimelineGlobals.kMaxTimelineValue;
            
            if ( GetIO().KeyShift || GetIO().KeyCtrl )
            {
                float period = 1.0f;
                if ( GetIO().KeyCtrl ) period *= 0.25f;
                
                newTime = Snap ( newTime, period );
            }
            
            *kTimelineGlobals.kCurrentTimelineValue = newTime;
            
            ui::SetTooltip( "%0.2f", newTime );
        }
        
        {
            float percComplete = *kTimelineGlobals.kCurrentTimelineValue / *kTimelineGlobals.kMaxTimelineValue;

            ImVec2 a = localOffset + ImVec2( lerp ( x0, x1, percComplete ), y0 );
            ImVec2 b = localOffset + ImVec2( a.x, y1 );
            
            draw->AddLine ( a, b, activeLineColor );
        }
    }
}