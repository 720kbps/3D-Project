#include "chase_camera.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_transform.hpp>
DISABLE_WARNINGS_POP()

glm::mat4 ChaseCamera::viewMatrix(const glm::vec3& cartPosition) const
{
    // sit behind the cart, on the side away from the center
    const glm::vec3 awayFromTarget = glm::normalize(cartPosition - target);
    const glm::vec3 position = cartPosition + distanceBehind * awayFromTarget + glm::vec3(0.0f, heightAbove, 0.0f);

    // look at the center
    return glm::lookAt(position, target, glm::vec3(0.0f, 1.0f, 0.0f));
}
