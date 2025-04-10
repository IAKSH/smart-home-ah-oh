// ServerStore.ts
import { Server } from '../models/Server';

export let activeServer: Server = {
  name: "",
  ip: "",
  description: "",
  icon: ""
};

export function setActiveServer(server: Server): void {
  activeServer = server;
}
