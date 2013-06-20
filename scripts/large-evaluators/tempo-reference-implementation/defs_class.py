
class Defs:
    def __init__(self):
        self.BPM_MIN = 40
        self.BPM_MAX = 180  # marsyas
        #self.BPM_MAX = 300
        #self.BPM_MAX = 1000
        
        self.OSS_WINDOWSIZE = 256
        self.OSS_HOPSIZE = 128
        self.OSS_WINDOWSIZE = 512
        self.OSS_HOPSIZE = 256
        
        self.OSS_LOWPASS_CUTOFF = 10.0 # Hz
        #self.OSS_LOWPASS_CUTOFF = 0 # Hz
        self.OSS_LOWPASS_N = 16
        
        self.BH_WINDOWSIZE = 2048
        self.BH_HOPSIZE = 128
        self.BH_WINDOWSIZE = 1024
        self.BH_HOPSIZE = 64
        
        self.BP_WINDOWSIZE = 2048
        self.BP_HOPSIZE = 128
        self.BP_WINDOWSIZE = 1024
        self.BP_HOPSIZE = 64
 
        self.WRITE_ONSETS = 1
        self.WRITE_BH = 1
        self.WRITE_BP = 1

        self.OPTIONS_ONSET = 2
        self.OPTIONS_BH = 2
        self.OPTIONS_BP = 2
        
