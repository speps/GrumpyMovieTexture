#import <AVFoundation/AVAudioSession.h>

void AudioSessionSetup()
{
    NSError *error = nil;
    AVAudioSession *session = [AVAudioSession sharedInstance];
    [session setCategory:AVAudioSessionCategoryPlayback error:&error];
}
