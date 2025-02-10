import { createRouter, createWebHistory } from 'vue-router'
import HomeView from '../views/HomeView.vue'
import ContentView from '../views/ContentView.vue'
import ContentDetailsView from '../views/ContentDetailsView.vue'
import TagView from '../views/TagView.vue'

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      name: 'home',
      component: HomeView,
    },
    {
      path: '/content',
      name: 'content',
      component: ContentView
    },
    {
      path: '/contentdetails/:contentRef',
      component: ContentDetailsView
    },
    {
      path: '/tags',
      name: 'tags',
      component: TagView
    },
  ],
})

export default router
