-- premake5.lua
workspace "PhotoViewer"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "PhotoViewer"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
include "Walnut/WalnutExternal.lua"

include "PhotoViewer"