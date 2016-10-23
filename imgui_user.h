//
//  imgui_user.h
//  Snepo
//
//  Created by Andrew Wright on 19/09/2016.
//
//

#pragma once

namespace ImGui
{
    IMGUI_API bool  ColorPicker         ( const char * label, float rgb[3] );
    
    IMGUI_API void  BeginTimeline       ( const char * str_id, float * currentTime, float * totalTime );
    IMGUI_API bool  TimelineEvent       ( const char * str_id, float * values, int numValues = 2 );
    IMGUI_API void  EndTimeline         ( );
    
    ImDrawList *    GetOverlayDrawList  ( );
    
    #if __has_include(<Snepo/Lucy.h>)
    
    IMGUI_API void  SceneGraph          ( bool forceAllOn = false );
    IMGUI_API void  InstallLogger       ( );
    IMGUI_API void  Logger              ( );
    
    #endif
}