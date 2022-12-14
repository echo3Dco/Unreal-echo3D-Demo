# echo3D Unreal Demo Scene
This demo scene was built using Unreal 4.27. All 3D content is streamed from the echo3D platform.


## Running the scene
Open the `Echo3DDemo` level if it does not open automatically. Press `Play`. The SDK will initialize and stream assets into a scene.


## Classes Overview
This section outlines the purpose and function of blueprints created for the demo scene.

### Level Blueprint
The `Echo3DDemo` level blueprint is the entry point for level activity. The SDK is initialized before calling functions on blueprint actors to trigger loading of their holograms.

### Actor Blueprints
These blueprints implement a base template to load holograms and place them into the scene at runtime.

#### BP_CircleRoom
This actor loads the circular room model. 

#### BP_Pedestal
This actor loads the eight pedestals and places them around the circular room.

#### BP_Artifact
This blueprint actor loads eight unique test holograms and places them above the pedestal. Each hologram is configured with a specific transform based on the model settings.

#### BP_WallPainting
This blueprint actor loads the picture frames as well as the paintings (images) and places them onto the walls of the circular room. 

### Template Blueprints

#### BP_DefaultMaterialTemplate
This template specifies details about the material that will be applied to loaded holograms. It applies the standard `M_EchoMaster` material which should work for most holograms. All demo holograms utilize this template. 

#### BP_HologramTransformTemplate
This template adds a transform variable which is used to instantiate holograms at a specified location. All scene holograms load using this template. 
