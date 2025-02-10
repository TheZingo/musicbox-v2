<template>
    <div class="battery-status">
      <div class="battery-icon">
        <svg v-if="props.isCharging" xmlns="http://www.w3.org/2000/svg" viewBox="9 0 9 24" width="9" height="24">
          <path d="M13 3v8h3l-4 8v-8H9l4-8z" fill="black"/>
        </svg>
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
          <rect x="2" y="7" width="20" height="10" fill="none" stroke="black" stroke-width="2" />
          <rect x="2" y="7" :width="batteryFillWidth" height="10" :fill="batteryFillColor" />
          <rect x="22" y="9" width="2" height="6" fill="black" />
        </svg>
      </div>
      <div class="battery-percentage">{{ formattedBatteryLevel }}%</div>
    </div>
  </template>
  
  <script setup lang="ts">
  import { computed } from 'vue';
  
  const props = defineProps<{
    batteryLevel: number,
    isCharging: boolean
  }>();
  
  const batteryFillColor = computed(() => {
    return props.batteryLevel > 20 ? 'green' : 'red';
  });
  
  const batteryFillWidth = computed(() => {
    return (props.batteryLevel / 100) * 20;
  });
  
  const formattedBatteryLevel = computed(() => {
    return props.batteryLevel.toFixed(1);
  });
  </script>
  
  <style scoped>
  .battery-status {
    display: flex;
    align-items: center;
  }
  
  .battery-icon svg {
    margin-right: 20px;
    scale: 1.5;
  }
  
  .battery-percentage {
    font-size: 1.2em;
  }

  @media (max-width: 500px) {
    .battery-percentage {
      display: none;
    }
  }
  </style>