#!/usr/bin/env python3
"""Regenerate the standalone Xcode project without external Python packages."""
from pathlib import Path
import hashlib,plistlib
ROOT=Path(__file__).resolve().parent
objects={}
def ident(label):return hashlib.sha1(label.encode()).hexdigest()[:24].upper()
def obj(label,isa,**kwargs):
 key=ident(label);objects[key]={'isa':isa,**kwargs};return key
def file(path,type,source='<group>'):
 return obj('file:'+path,'PBXFileReference',path=path,lastKnownFileType=type,sourceTree=source)
def build(ref,flags=None):
 kw={'fileRef':ref}
 if flags:kw['settings']={'COMPILER_FLAGS':flags}
 return obj('build:'+ref,'PBXBuildFile',**kw)
app_sources=['App/main.m','App/CutoutController.m','App/CutoutBridge.mm']
native=ROOT.parent/'native'
core=['src/core.cpp','src/platform.cpp','legacy/KernelEstimation.cpp','legacy/Dijkstra.cpp',*[str(p.relative_to(native)) for p in sorted((native/'legacy/mw').glob('*.cpp'))]]
vendor=[str(p.relative_to(native)) for folder in ['vendor/ann','vendor/figtree'] for p in sorted((native/folder).glob('*.cpp'))]
source_refs=[];source_builds=[]
for path in app_sources+['../native/'+p for p in core+vendor]:
 ref=file(path,'sourcecode.cpp.objcpp' if path.endswith('.mm') else 'sourcecode.c.objc' if path.endswith('.m') else 'sourcecode.cpp.cpp');source_refs.append(ref);source_builds.append(build(ref,'-std=c++14' if '/vendor/' in path else None))
resources=[];resource_builds=[]
for path in ['../validation/fixtures/coffee.png','../validation/fixtures/coffee-scribbles.json','Resources/credits.txt']:
 ref=file(path,'image.png' if path.endswith('.png') else 'text.json' if path.endswith('.json') else 'text');resources.append(ref);resource_builds.append(build(ref))
# Folder reference preserves license subdirectories in the bundle.
licenses=file('Resources/Licenses','folder');resources.append(licenses);resource_builds.append(build(licenses))
frameworks=[];framework_builds=[]
for name in ['UIKit','Foundation','PhotosUI','ImageIO','CoreGraphics','QuartzCore','Accelerate','UniformTypeIdentifiers']:
 ref=file('System/Library/Frameworks/'+name+'.framework','wrapper.framework','SDKROOT');frameworks.append(ref);framework_builds.append(build(ref))
product=file('CutOutUpdated.app','wrapper.application','BUILT_PRODUCTS_DIR')
sources=obj('sources','PBXSourcesBuildPhase',buildActionMask=2147483647,files=source_builds,runOnlyForDeploymentPostprocessing=0)
res=obj('resources','PBXResourcesBuildPhase',buildActionMask=2147483647,files=resource_builds,runOnlyForDeploymentPostprocessing=0)
fw=obj('frameworks','PBXFrameworksBuildPhase',buildActionMask=2147483647,files=framework_builds,runOnlyForDeploymentPostprocessing=0)
settings={'SDKROOT':'iphoneos','IPHONEOS_DEPLOYMENT_TARGET':'16.0','TARGETED_DEVICE_FAMILY':'1,2','CLANG_ENABLE_MODULES':'YES','CLANG_ENABLE_OBJC_ARC':'YES','CLANG_CXX_LANGUAGE_STANDARD':'c++17','CLANG_CXX_LIBRARY':'libc++','CODE_SIGN_STYLE':'Automatic','DEVELOPMENT_TEAM':'','CUTOUT_BUNDLE_IDENTIFIER':'org.example.cutout.updated','PRODUCT_BUNDLE_IDENTIFIER':'$(CUTOUT_BUNDLE_IDENTIFIER)','PRODUCT_NAME':'CutOutUpdated','INFOPLIST_FILE':'App/Info.plist','FRAMEWORK_SEARCH_PATHS':['$(inherited)','$(OPENCV_IOS_FRAMEWORK_DIR)'],'HEADER_SEARCH_PATHS':['$(inherited)','$(SRCROOT)/../native/include','$(SRCROOT)/../native/legacy','$(SRCROOT)/../native/vendor/include','$(SRCROOT)/../native/vendor/include/figtree','$(SRCROOT)/../native/src'],'OTHER_LDFLAGS':['$(inherited)','-framework','opencv2','-lc++','-lz'],'GCC_PREPROCESSOR_DEFINITIONS':['$(inherited)','_LIBCPP_ENABLE_CXX17_REMOVED_RANDOM_SHUFFLE=1'],'GCC_WARN_INHIBIT_ALL_WARNINGS':'NO','WARNING_CFLAGS':['-Wno-write-strings','-Wno-register'],'COMPRESS_PNG_FILES':'NO','STRIP_PNG_TEXT':'NO','ENABLE_BITCODE':'NO','ENABLE_USER_SCRIPT_SANDBOXING':'YES','SUPPORTED_PLATFORMS':'iphoneos iphonesimulator'}
project_settings={key:settings[key] for key in ['SDKROOT','IPHONEOS_DEPLOYMENT_TARGET','TARGETED_DEVICE_FAMILY','CLANG_ENABLE_MODULES','CLANG_ENABLE_OBJC_ARC','CLANG_CXX_LANGUAGE_STANDARD','CLANG_CXX_LIBRARY','CODE_SIGN_STYLE','DEVELOPMENT_TEAM','CUTOUT_BUNDLE_IDENTIFIER','SUPPORTED_PLATFORMS']}
project_configs=[obj('project-cfg:'+name,'XCBuildConfiguration',name=name,buildSettings=project_settings) for name in ['Debug','Release']]
project_config_list=obj('project-cfg-list','XCConfigurationList',buildConfigurations=project_configs,defaultConfigurationIsVisible=0,defaultConfigurationName='Debug')
configs=[]
for name in ['Debug','Release']:
 cfg={key:value for key,value in settings.items() if key not in project_settings};cfg.update({'GCC_OPTIMIZATION_LEVEL':'0' if name=='Debug' else '3','DEBUG_INFORMATION_FORMAT':'dwarf' if name=='Debug' else 'dwarf-with-dsym','ONLY_ACTIVE_ARCH':'YES' if name=='Debug' else 'NO'});configs.append(obj('cfg:'+name,'XCBuildConfiguration',name=name,buildSettings=cfg))
config_list=obj('cfg-list','XCConfigurationList',buildConfigurations=configs,defaultConfigurationIsVisible=0,defaultConfigurationName='Debug')
target=obj('app-target','PBXNativeTarget',name='CutOutUpdated',productName='CutOutUpdated',productReference=product,productType='com.apple.product-type.application',buildConfigurationList=config_list,buildPhases=[sources,fw,res],dependencies=[],buildRules=[])
test_ref=file('Tests/CutOutUITests.m','sourcecode.c.objc')
test_product=file('CutOutUITests.xctest','wrapper.cfbundle','BUILT_PRODUCTS_DIR')
test_sources=obj('test-sources','PBXSourcesBuildPhase',buildActionMask=2147483647,files=[build(test_ref)],runOnlyForDeploymentPostprocessing=0)
test_frameworks=obj('test-frameworks','PBXFrameworksBuildPhase',buildActionMask=2147483647,files=[],runOnlyForDeploymentPostprocessing=0)
test_configs=[]
for name in ['Debug','Release']:
    cfg={}
    cfg.update({'PRODUCT_NAME':'CutOutUITests','PRODUCT_BUNDLE_IDENTIFIER':'$(CUTOUT_BUNDLE_IDENTIFIER).UITests','GENERATE_INFOPLIST_FILE':'YES','TEST_TARGET_NAME':'CutOutUpdated','FRAMEWORK_SEARCH_PATHS':['$(inherited)','$(PLATFORM_DIR)/Developer/Library/Frameworks'],'OTHER_LDFLAGS':['$(inherited)','-framework','XCTest'],'LD_RUNPATH_SEARCH_PATHS':['$(inherited)','@executable_path/Frameworks','@loader_path/Frameworks']})
    test_configs.append(obj('test-cfg:'+name,'XCBuildConfiguration',name=name,buildSettings=cfg))
test_config_list=obj('test-cfg-list','XCConfigurationList',buildConfigurations=test_configs,defaultConfigurationIsVisible=0,defaultConfigurationName='Debug')
proxy=obj('test-proxy','PBXContainerItemProxy',containerPortal=ident('project'),proxyType=1,remoteGlobalIDString=target,remoteInfo='CutOutUpdated')
dependency=obj('test-dependency','PBXTargetDependency',target=target,targetProxy=proxy)
test_target=obj('test-target','PBXNativeTarget',name='CutOutUITests',productName='CutOutUITests',productReference=test_product,productType='com.apple.product-type.bundle.ui-testing',buildConfigurationList=test_config_list,buildPhases=[test_sources,test_frameworks],dependencies=[dependency],buildRules=[])
products=obj('products' ,'PBXGroup',name='Products',children=[product,test_product],sourceTree='<group>')
main=obj('main','PBXGroup',children=source_refs+resources+frameworks+[test_ref,products],sourceTree='<group>')
project=obj('project','PBXProject',attributes={'LastUpgradeCheck':'1610','TargetAttributes':{test_target:{'TestTargetID':target}}},buildConfigurationList=project_config_list,compatibilityVersion='Xcode 14.0',developmentRegion='en',knownRegions=['en','Base'],mainGroup=main,productRefGroup=products,projectDirPath='',projectRoot='',targets=[target,test_target])
(ROOT/'CutOutUpdated.xcodeproj/project.pbxproj').write_bytes(plistlib.dumps({'archiveVersion':'1','classes':{},'objectVersion':'56','objects':objects,'rootObject':project},sort_keys=False))
scheme=f'''<?xml version="1.0" encoding="UTF-8"?><Scheme LastUpgradeVersion="1610" version="1.3"><BuildAction parallelizeBuildables="NO" buildImplicitDependencies="YES"><BuildActionEntries><BuildActionEntry buildForTesting="YES" buildForRunning="YES" buildForProfiling="YES" buildForArchiving="YES" buildForAnalyzing="YES"><BuildableReference BuildableIdentifier="primary" BlueprintIdentifier="{target}" BuildableName="CutOutUpdated.app" BlueprintName="CutOutUpdated" ReferencedContainer="container:CutOutUpdated.xcodeproj"/></BuildActionEntry><BuildActionEntry buildForTesting="YES" buildForRunning="NO" buildForProfiling="NO" buildForArchiving="NO" buildForAnalyzing="NO"><BuildableReference BuildableIdentifier="primary" BlueprintIdentifier="{test_target}" BuildableName="CutOutUITests.xctest" BlueprintName="CutOutUITests" ReferencedContainer="container:CutOutUpdated.xcodeproj"/></BuildActionEntry></BuildActionEntries></BuildAction><TestAction buildConfiguration="Debug" selectedDebuggerIdentifier="Xcode.DebuggerFoundation.Debugger.LLDB" selectedLauncherIdentifier="Xcode.IDEFoundation.Launcher.LLDB" shouldUseLaunchSchemeArgsEnv="YES"><Testables><TestableReference skipped="NO"><BuildableReference BuildableIdentifier="primary" BlueprintIdentifier="{test_target}" BuildableName="CutOutUITests.xctest" BlueprintName="CutOutUITests" ReferencedContainer="container:CutOutUpdated.xcodeproj"/></TestableReference></Testables></TestAction><LaunchAction buildConfiguration="Debug" selectedDebuggerIdentifier="Xcode.DebuggerFoundation.Debugger.LLDB" selectedLauncherIdentifier="Xcode.IDEFoundation.Launcher.LLDB" launchStyle="0" useCustomWorkingDirectory="NO" ignoresPersistentStateOnLaunch="NO" debugDocumentVersioning="YES" allowLocationSimulation="NO"><BuildableProductRunnable runnableDebuggingMode="0"><BuildableReference BuildableIdentifier="primary" BlueprintIdentifier="{target}" BuildableName="CutOutUpdated.app" BlueprintName="CutOutUpdated" ReferencedContainer="container:CutOutUpdated.xcodeproj"/></BuildableProductRunnable></LaunchAction><AnalyzeAction buildConfiguration="Debug"/><ArchiveAction buildConfiguration="Release" revealArchiveInOrganizer="YES"/></Scheme>'''
(ROOT/'CutOutUpdated.xcodeproj/xcshareddata/xcschemes/CutOutUpdated.xcscheme').write_text(scheme+'\n')
print('Generated Xcode project with',len(source_builds),'source files.')
