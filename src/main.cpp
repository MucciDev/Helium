#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/CCNode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/ShaderLayer.hpp>
#include <Geode/modify/CCFileUtils.hpp>
#include <thread>
#include <unordered_map>
#include <chrono>

#ifdef GEODE_IS_WINDOWS
#include <windows.h>
#endif

using namespace geode::prelude;

// Global State
CCRect g_cameraBounds;
bool g_suspendSorting = false;
std::chrono::steady_clock::time_point g_bootStartTime; // TIMER START

// ==========================================
// FAST FORMAT (MATCOOL IMPLEMENTATION)
// ==========================================
// Bypasses the fuckass slow Cocos2d-x string formatter.
bool fastFormatHook(cocos2d::CCString* self, const char* format, va_list ap) {
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int size = std::vsnprintf(nullptr, 0, format, ap_copy);
    va_end(ap_copy);
    
    std::string str(size + 1, '\0');
    std::vsnprintf(str.data(), str.size(), format, ap);
    self->m_sString = str;
    return true;
}

$execute {
    g_bootStartTime = std::chrono::steady_clock::now(); // START THE STOPWATCH
    
    #ifdef GEODE_IS_WINDOWS
    auto addr = GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?initWithFormatAndValist@CCString@cocos2d@@AAE_NPBDPAD@Z");
    if (addr) {
        (void) Mod::get()->hook(
            reinterpret_cast<void*>(addr),
            &fastFormatHook,
            "cocos2d::CCString::initWithFormatAndValist",
            tulip::hook::TulipConvention::Thiscall
        );
    }
    #endif
}

// ==========================================
// THREAD-LOCAL I/O CACHE
// ==========================================
// Eradicates disk search latency by caching resolved file paths per-thread.
class $modify(OptimizedFileUtils, CCFileUtils) {
    gd::string fullPathForFilename(const char* pszFileName, bool bResolutionDirectory) {
        if (!pszFileName || strlen(pszFileName) == 0) return "";
        thread_local std::unordered_map<std::string, std::string> tls_pathCache;
        std::string key = std::string(pszFileName) + (bResolutionDirectory ? "_1" : "_0");
        auto it = tls_pathCache.find(key);
        if (it != tls_pathCache.end()) return it->second.c_str();

        gd::string result = CCFileUtils::fullPathForFilename(pszFileName, bResolutionDirectory);
        tls_pathCache[key] = result.c_str();
        return result;
    }
};

// ==========================================
// CGYTRUS SHADER CACHE
// ==========================================
// Prevents redundant OpenGL shader recompiles.
class $modify(OptimizedShaderLayer, ShaderLayer) {
    void setupShader(bool shouldReset) {
        if (!shouldReset && this->m_shader) return; 
        ShaderLayer::setupShader(shouldReset);
    }
};

// ==========================================
// INVISIBLE BOOT
// ==========================================
class $modify(FastBootLoadingLayer, LoadingLayer) {
    bool init(bool fromReload) {
        // Cut texture bandwidth in half during boot
        cocos2d::CCTexture2D::setDefaultAlphaPixelFormat(kCCTexture2DPixelFormat_RGBA4444);
        
        if (!LoadingLayer::init(fromReload)) return false;
        
        CCApplication::sharedApplication()->toggleVerticalSync(false);
        CCDirector::sharedDirector()->setAnimationInterval(0.0f);
        this->setVisible(false);
        return true;
    }
};

class $modify(FastBootMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        
        // STOP THE STOPWATCH
        auto bootEndTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(bootEndTime - g_bootStartTime);
        
        log::info("==========================================");
        log::info("[Helium] BOOT COMPLETE IN {}ms ({}s)", duration.count(), duration.count() / 1000.0);
        log::info("==========================================");

        cocos2d::CCTexture2D::setDefaultAlphaPixelFormat(kCCTexture2DPixelFormat_Default);
        auto gm = GameManager::sharedState();
        if (gm->getGameVariable("0030")) CCApplication::sharedApplication()->toggleVerticalSync(true);
        float targetFPS = gm->m_customFPSTarget == 0 ? 60.0f : gm->m_customFPSTarget;
        CCDirector::sharedDirector()->setAnimationInterval(1.0 / targetFPS);
        
        return true;
    }
};

// ==========================================
// ASYNC DATA LOAD
// ==========================================
class $modify(LithiumGameManager, GameManager) {
    void loadDataFromFile(gd::string const& filename) {
        if (filename == "CCLocalLevels.dat" || filename == "CCGameManager.dat") {
            std::thread([this, filename]() {
                GameManager::loadDataFromFile(filename);
            }).detach();
            return; 
        }
        GameManager::loadDataFromFile(filename);
    }
};

// ==========================================
// GAMEPLAY-SAFE VISUAL CULLING
// ==========================================
class $modify(OptimizedDirector, CCDirector) {
    void drawScene() {
        auto winSize = this->getWinSize();
        g_cameraBounds.origin = CCPoint(-300.0f, -300.0f);
        g_cameraBounds.size = CCSize(winSize.width + 600.0f, winSize.height + 600.0f);
        CCDirector::drawScene();
    }
};

class $modify(OptimizedGameObject, GameObject) {
    void visit() {
        if (!this->m_bVisible) return;
        if (this->m_objectType == GameObjectType::Decoration) {
            CCPoint screenPos = this->convertToWorldSpace(CCPointZero);
            if (!g_cameraBounds.containsPoint(screenPos)) return; 
        }
        GameObject::visit();
    }
};

// ==========================================
// Z-ORDER SORTING OPTIMIZER
// ==========================================
class $modify(OptimizedNode, CCNode) {
    void sortAllChildren() {
        if (g_suspendSorting) {
            this->m_bReorderChildDirty = true;
            return;
        }
        CCNode::sortAllChildren();
    }
};

class $modify(OptimizedPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        g_suspendSorting = true;
        bool result = PlayLayer::init(level, useReplay, dontCreateObjects);
        g_suspendSorting = false;
        if (this->m_objectLayer) this->m_objectLayer->sortAllChildren();
        return result;
    }
};

// ==========================================
// OS PRIORITY
// ==========================================
$on_mod(Loaded) {
    #ifdef GEODE_IS_WINDOWS
    HANDLE hProcess = GetCurrentProcess();
    SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    #endif
}