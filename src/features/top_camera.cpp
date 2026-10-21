#include "top_camera.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_transform.hpp>
DISABLE_WARNINGS_POP()

glm::mat4 TopCamera::viewMatrix() const
{
    // look straight down, with -Z as "up" on screen
    return glm::lookAt(glm::vec3(0.0f, height, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::mat4 TopCamera::projectionMatrix(float aspectRatio) const
{
    // orthographic box around the scene
    return glm::ortho(-viewSize * aspectRatio, viewSize * aspectRatio, -viewSize, viewSize, 0.1f, height + 5.0f);
}
