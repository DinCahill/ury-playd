# Visual Studio Build Notes

Visual Studio is a fickle beast, and as of writing the resources provided in
the playd repository won't be enough to build from scratch.  Until we
can sort everything out, here are some field notes.

## Changing Paths

Currently, the paths given in `playd.vcxproj` are absolute, and will
need to be changed to suit your build environment.  This may be fixed later.

## PortAudio

Use cmake, and the shared library (`portaudio-x86.lib`).

The C++ bindings are _not_ built by the cmake-generated Visual Studio
solution.  However, the Visual Studio 7.1 solution in
`bindings\cpp\build\vc7_1\static_library.sln` should work once upgraded to
modern Visual Studio.

## LibUV

This _must_ be built as a shared library (`vcbuild.bat shared`).  Otherwise,
this should work fine.

## SoX

Some persuasion of the dated sources recommended by the SoX build instructions
is necessary to get them to build with modern Visual Studio.  The following
'hacks' work on 32-bit VS2013:

* libflac: Replace all `ftello`/`fseeko` with `ftell`/`fseek`;
* libsndfile: Replace all lrint with _lrint.
* In the LibSox project, change the following properties:
  * __General/General/Target Extension__ to __.dll__;
  * __General/Project Defaults/Configuration Type__ to
    __Dynamic Library (.dll)__;
  * __Linker/Input/Additional Dependencies__ should contain:
    * `winmm.lib`
    * `libflac.lib`
    * `libgsm.lib`
    * `libid3tag.lib`
    * `liblpc10.lib`
    * `libmad.lib`
    * `libmp3lame.lib`
    * `libogg.lib`
    * `libpng.lib`
    * `libsndfileg72x.lib`
    * `libsndfile-1.lib`
    * `libsndfilegsm610.lib`
    * `libspeex.lib`
    * `libvorbis.lib`
    * `libwavpack.lib`
    * `libzlib.lib`
  * __Linker/Input/Module Definition File__ should point to a file with the
    contents given below;
  * __Linker/General/Link Library Dependencies__ may need to be set to __No__.

### Module definition file

This is needed to make LibSoX build a `.lib` file for dynamic linking.
Something like the below should work:

```
EXPORTS
_lsx_ima_bytes_per_block
clear_fft_cache
init_fft_cache
lsx_13linear2alaw
lsx_14linear2ulaw
lsx_Gsm_Coder
lsx_Gsm_Decoder
lsx_Gsm_LPC_Analysis
lsx_Gsm_Long_Term_Predictor
lsx_Gsm_Long_Term_Synthesis_Filtering
lsx_Gsm_Preprocess
lsx_Gsm_RPE_Decoding
lsx_Gsm_RPE_Encoding
lsx_Gsm_Short_Term_Analysis_Filter
lsx_Gsm_Short_Term_Synthesis_Filter
lsx_adpcm_decode
lsx_adpcm_encode
lsx_adpcm_flush
lsx_adpcm_ima_start
lsx_adpcm_init
lsx_adpcm_oki_start
lsx_adpcm_read
lsx_adpcm_reset
lsx_adpcm_stopread
lsx_adpcm_stopwrite
lsx_adpcm_write
lsx_aifc_format_fn
lsx_aifcstartwrite
lsx_aifcstopwrite
lsx_aiff_format_fn
lsx_aiffstartread
lsx_aiffstartwrite
lsx_aiffstopread
lsx_aiffstopwrite
lsx_al_format_fn
lsx_alaw2linear16
lsx_allpass_effect_fn
lsx_apply_bartlett
lsx_apply_blackman
lsx_apply_blackman_nutall
lsx_apply_hamming
lsx_apply_hann
lsx_apply_hann_f
lsx_apply_kaiser
lsx_au_format_fn
lsx_avr_format_fn
lsx_band_effect_fn
lsx_bandpass_effect_fn
lsx_bandreject_effect_fn
lsx_bass_effect_fn
lsx_bend_effect_fn
lsx_bessel_I_0
lsx_biquad_effect_fn
lsx_biquad_flow
lsx_biquad_getopts
lsx_biquad_start
lsx_cat_comments
lsx_cdft
lsx_cdr_format_fn
lsx_channels_effect_fn
lsx_check_read_params
lsx_chorus_effect_fn
lsx_clearerr
lsx_close_dllibrary
lsx_compand_effect_fn
lsx_compandt_kill
lsx_compandt_parse
lsx_compandt_show
lsx_contrast_effect_fn
lsx_cvsd_format_fn
lsx_cvsdread
lsx_cvsdstartread
lsx_cvsdstartwrite
lsx_cvsdstopread
lsx_cvsdstopwrite
lsx_cvsdwrite
lsx_cvu_format_fn
lsx_dat_format_fn
lsx_dcshift_effect_fn
lsx_ddct
lsx_ddst
lsx_debug_impl
lsx_debug_more_impl
lsx_debug_most_impl
lsx_deemph_effect_fn
lsx_delay_effect_fn
lsx_design_lpf
lsx_dfct
lsx_dfst
lsx_dft_filter_effect_fn
lsx_dither_effect_fn
lsx_divide_effect_fn
lsx_downsample_effect_fn
lsx_dvms_format_fn
lsx_dvmsstartread
lsx_dvmsstartwrite
lsx_dvmsstopwrite
lsx_earwax_effect_fn
lsx_echo_effect_fn
lsx_echos_effect_fn
lsx_effect_set_imin
lsx_effects_init
lsx_effects_quit
lsx_enum_option
lsx_eof
lsx_equalizer_effect_fn
lsx_error
lsx_f4_format_fn
lsx_f8_format_fn
lsx_fade_effect_fn
lsx_fail_errno
lsx_fail_impl
lsx_filelength
lsx_find_enum_text
lsx_find_enum_value
lsx_find_file_extension
lsx_fir_effect_fn
lsx_fir_to_phase
lsx_firfit_effect_fn
lsx_flanger_effect_fn
lsx_flow_copy
lsx_flush
lsx_g721_decoder
lsx_g721_encoder
lsx_g723_24_decoder
lsx_g723_24_encoder
lsx_g723_40_decoder
lsx_g723_40_encoder
lsx_g72x_init_state
lsx_g72x_predictor_pole
lsx_g72x_predictor_zero
lsx_g72x_quantize
lsx_g72x_reconstruct
lsx_g72x_step_size
lsx_g72x_tandem_adjust_alaw
lsx_g72x_tandem_adjust_ulaw
lsx_g72x_update
lsx_gain_effect_fn
lsx_generate_wave_table
lsx_get_wave_enum
lsx_getopt
lsx_getopt_init
lsx_gsm_A
lsx_gsm_B
lsx_gsm_DLB
lsx_gsm_FAC
lsx_gsm_H
lsx_gsm_INVA
lsx_gsm_L_add
lsx_gsm_L_asl
lsx_gsm_L_asr
lsx_gsm_L_mult
lsx_gsm_L_sub
lsx_gsm_MAC
lsx_gsm_MIC
lsx_gsm_NRFAC
lsx_gsm_QLB
lsx_gsm_abs
lsx_gsm_add
lsx_gsm_asl
lsx_gsm_asr
lsx_gsm_create
lsx_gsm_decode
lsx_gsm_destroy
lsx_gsm_div
lsx_gsm_encode
lsx_gsm_format_fn
lsx_gsm_mult
lsx_gsm_mult_r
lsx_gsm_norm
lsx_gsm_option
lsx_gsm_sub
lsx_gsrt_format_fn
lsx_hcom_format_fn
lsx_highpass_effect_fn
lsx_hilbert_effect_fn
lsx_htk_format_fn
lsx_ima_block_expand_i
lsx_ima_block_expand_m
lsx_ima_block_mash_i
lsx_ima_bytes_per_block
lsx_ima_format_fn
lsx_ima_init_table
lsx_ima_samples_in
lsx_ima_start
lsx_input_effect_fn
lsx_kaiser_beta
lsx_la_format_fn
lsx_loudness_effect_fn
lsx_lowpass_effect_fn
lsx_lpc10_analys_
lsx_lpc10_bsynz_
lsx_lpc10_chanrd_
lsx_lpc10_chanwr_
lsx_lpc10_contrl_
lsx_lpc10_create_decoder_state
lsx_lpc10_create_encoder_state
lsx_lpc10_dcbias_
lsx_lpc10_decode
lsx_lpc10_decode_
lsx_lpc10_deemp_
lsx_lpc10_difmag_
lsx_lpc10_dyptrk_
lsx_lpc10_encode
lsx_lpc10_encode_
lsx_lpc10_energy_
lsx_lpc10_format_fn
lsx_lpc10_ham84_
lsx_lpc10_hp100_
lsx_lpc10_i_nint
lsx_lpc10_init_decoder_state
lsx_lpc10_init_encoder_state
lsx_lpc10_invert_
lsx_lpc10_irc2pc_
lsx_lpc10_ivfilt_
lsx_lpc10_lpcini_
lsx_lpc10_lpfilt_
lsx_lpc10_median_
lsx_lpc10_mload_
lsx_lpc10_onset_
lsx_lpc10_pitsyn_
lsx_lpc10_placea_
lsx_lpc10_placev_
lsx_lpc10_pow_ii
lsx_lpc10_preemp_
lsx_lpc10_prepro_
lsx_lpc10_r_sign
lsx_lpc10_random_
lsx_lpc10_rcchk_
lsx_lpc10_synths_
lsx_lpc10_tbdm_
lsx_lpc10_voicin_
lsx_lpc10_vparms_
lsx_lpf_num_taps
lsx_lu_format_fn
lsx_make_lpf
lsx_maud_format_fn
lsx_mcompand_effect_fn
lsx_mixer_effect_fn
lsx_ms_adpcm_block_expand_i
lsx_ms_adpcm_block_mash_i
lsx_ms_adpcm_bytes_per_block
lsx_ms_adpcm_i_coef
lsx_ms_adpcm_samples_in
lsx_noiseprof_effect_fn
lsx_noisered_effect_fn
lsx_norm_effect_fn
lsx_nul_format_fn
lsx_offset_seek
lsx_oops_effect_fn
lsx_open_dllibrary
lsx_open_input_file
lsx_output_effect_fn
lsx_overdrive_effect_fn
lsx_pad_effect_fn
lsx_padbytes
lsx_parse_frequency_k
lsx_parse_note
lsx_parsesamples
lsx_phaser_effect_fn
lsx_pitch_effect_fn
lsx_plot_fir
lsx_power_spectrum
lsx_power_spectrum_f
lsx_prc_format_fn
lsx_prepare_spline3
lsx_rate_effect_fn
lsx_raw_format_fn
lsx_rawread
lsx_rawseek
lsx_rawstart
lsx_rawwrite
lsx_rdft
lsx_read3
lsx_read_3_buf
lsx_read_b_buf
lsx_read_df_buf
lsx_read_dw_buf
lsx_read_f_buf
lsx_read_qw_buf
lsx_read_w_buf
lsx_readb
lsx_readbuf
lsx_readchars
lsx_readdf
lsx_readdw
lsx_readf
lsx_readqw
lsx_reads
lsx_readw
lsx_realloc
lsx_remix_effect_fn
lsx_repeat_effect_fn
lsx_report_impl
lsx_reverb_effect_fn
lsx_reverse_effect_fn
lsx_rewind
lsx_riaa_effect_fn
lsx_s1_format_fn
lsx_s2_format_fn
lsx_s3_format_fn
lsx_s4_format_fn
lsx_safe_cdft
lsx_safe_rdft
lsx_seeki
lsx_set_dft_filter
lsx_set_dft_length
lsx_set_signal_defaults
lsx_sf_format_fn
lsx_sigfigs3
lsx_sigfigs3p
lsx_silence_effect_fn
lsx_sinc_effect_fn
lsx_skipbytes
lsx_sln_format_fn
lsx_smp_format_fn
lsx_sounder_format_fn
lsx_soundtool_format_fn
lsx_sox_format_fn
lsx_speed_effect_fn
lsx_sphere_format_fn
lsx_splice_effect_fn
lsx_spline3
lsx_stat_effect_fn
lsx_stats_effect_fn
lsx_strcasecmp
lsx_strends
lsx_stretch_effect_fn
lsx_strncasecmp
lsx_svx_format_fn
lsx_swap_effect_fn
lsx_synth_effect_fn
lsx_tell
lsx_tempo_effect_fn
lsx_tmpfile
lsx_treble_effect_fn
lsx_tremolo_effect_fn
lsx_trim_effect_fn
lsx_txw_format_fn
lsx_u1_format_fn
lsx_u2_format_fn
lsx_u3_format_fn
lsx_u4_format_fn
lsx_ul_format_fn
lsx_ulaw2linear16
lsx_unreadb
lsx_upsample_effect_fn
lsx_usage
lsx_usage_lines
lsx_vad_effect_fn
lsx_voc_format_fn
lsx_vol_effect_fn
lsx_vorbis_format_fn
lsx_vox_format_fn
lsx_vox_read
lsx_vox_start
lsx_vox_stopread
lsx_vox_stopwrite
lsx_vox_write
lsx_warn_impl
lsx_wav_format_fn
lsx_waveaudio_format_fn
lsx_wavpack_format_fn
lsx_write3
lsx_write_3_buf
lsx_write_b_buf
lsx_write_df_buf
lsx_write_dw_buf
lsx_write_f_buf
lsx_write_qw_buf
lsx_write_w_buf
lsx_writeb
lsx_writebuf
lsx_writedf
lsx_writedw
lsx_writef
lsx_writeqw
lsx_writes
lsx_writesb
lsx_writesw
lsx_writew
lsx_wve_format_fn
lsx_xa_format_fn
lt_dlclose
lt_dlerror
lt_dlexit
lt_dlforeachfile
lt_dlinit
lt_dlopen
lt_dlopenext
lt_dlsetsearchpath
lt_dlsym
sox_add_effect
sox_append_comment
sox_append_comments
sox_basename
sox_close
sox_copy_comments
sox_create_effect
sox_create_effects_chain
sox_delete_comments
sox_delete_effect
sox_delete_effect_last
sox_delete_effects
sox_delete_effects_chain
sox_effect_options
sox_effects_clips
sox_find_comment
sox_find_effect
sox_find_format
sox_flow_effects
sox_format_init
sox_format_quit
sox_format_supports_encoding
sox_get_effect_fns
sox_get_effects_globals
sox_get_encodings_info
sox_get_format_fns
sox_get_globals
sox_init
sox_init_encodinginfo
sox_is_playlist
sox_num_comments
sox_open_mem_read
sox_open_mem_write
sox_open_memstream_write
sox_open_read
sox_open_write
sox_parse_playlist
sox_pop_effect_last
sox_precision
sox_push_effect_last
sox_quit
sox_read
sox_seek
sox_stop_effect
sox_strerror
sox_trim_clear_start
sox_trim_get_start
sox_version
sox_version_info
sox_write
sox_write_handler
```