/* Public domain. */

#include <linux/kernel.h>

#include <drm/drm_plane.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_gem.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <linux/dma-fence-chain.h>
#include <linux/dma-resv.h>

int
drm_gem_plane_helper_prepare_fb(struct drm_plane *dp,
    struct drm_plane_state *dps)
{
	struct drm_gem_object *obj;
	struct dma_fence *f1, *f2;
	struct dma_fence_chain *dfc;
	enum dma_resv_usage dru;
	unsigned int i;
	int r;

	f1 = dma_fence_get(dps->fence);
	if (dps->fb != NULL) {
		dru = f1 != NULL ? DMA_RESV_USAGE_KERNEL : DMA_RESV_USAGE_WRITE;
		for (i = 0; i < dps->fb->format->num_planes; i++) {
			obj = drm_gem_fb_get_obj(dps->fb, i);
			if (obj == NULL) {
				dma_fence_put(f1);
				return -EINVAL;
			}
			r = dma_resv_get_singleton(obj->resv, dru, &f2);
			if (r) {
				dma_fence_put(f1);
				return r;
			}
			if (f2 == NULL)
				continue;
			if (f1 != NULL) {
				dfc = dma_fence_chain_alloc();
				dma_fence_chain_init(dfc, f1, f2, 1);
				f1 = &dfc->base;
			} else
				f1 = f2;
		}
		dma_fence_put(dps->fence);
		dps->fence = f1;
	}

	return 0;
}
