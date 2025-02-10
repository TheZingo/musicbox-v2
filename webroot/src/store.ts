import { reactive } from 'vue'
import { type TagData } from './types/TagData'

export const store = reactive({
    hasTag: false,
    playState: false,
    currentTag: {} as TagData,
    batteryLevel: 0,
    batteryCharging: false,
  })

